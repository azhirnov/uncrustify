/**
 * @file source_layout.cpp
 * Source-preserving output uses the normal parser's computed columns, but only
 * edits source gaps. Original tokens, comments and literals are never emitted
 * from the transformed chunk list, so token-rewriting options have no effect.
 * @license GPL v2+
 */
#include "source_layout.h"

#include "align/tools.h"
#include "chunk.h"
#include "options.h"
#include "unicode.h"

#include <algorithm>
#include <map>
#include <set>

using namespace uncrustify;

namespace
{

bool horizontal(int ch)
{
   return(ch == ' ' || ch == '\t');
}


size_t advance(size_t column, int ch, size_t tab_size)
{
   if (ch == '\n' || ch == '\r')
   {
      return(1);
   }
   return(ch == '\t' ? column + tab_size - (column - 1) % tab_size : column + 1);
}


bool comment(E_Token type)
{
   return(  type == E_Token::CT_COMMENT || type == E_Token::CT_COMMENT_CPP
         || type == E_Token::CT_COMMENT_MULTI || type == E_Token::CT_IGNORED);
}


bool simplify_defined(std::vector<int> &label)
{
   const char *word = "defined";
   size_t     i     = 0;

   while (*word && i < label.size() && label[i] == *word)
   {
      ++i;
      ++word;
   }

   if (*word || i == label.size() || (!horizontal(label[i]) && label[i] != '('))
   {
      return(false);
   }
   std::vector<int> compact;

   while (i < label.size() && horizontal(label[i]))
   {
      ++i;
   }
   const bool parenthesized = i < label.size() && label[i] == '(';

   if (parenthesized)
   {
      ++i;
   }

   while (i < label.size() && horizontal(label[i]))
   {
      ++i;
   }

   for ( ; i < label.size(); ++i)
   {
      const int ch = label[i];

      if (!(ch == '_' || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (!compact.empty() && ch >= '0' && ch <= '9')))
      {
         break;
      }
      compact.push_back(ch);
   }

   while (i < label.size() && horizontal(label[i]))
   {
      ++i;
   }

   if (parenthesized)
   {
      if (i == label.size() || label[i] != ')')
      {
         return(false);
      }
      ++i;
   }

   while (i < label.size() && horizontal(label[i]))
   {
      ++i;
   }

   if (compact.empty() || i != label.size())
   {
      return(false);
   }
   label = compact;
   return(true);
} // simplify_defined

} // namespace

SourceLayout::SourceLayout(const std::deque<int> &data)
   : m_data(data)
   , m_lines(1, 0)
{
   for (size_t i = 0; i < data.size(); ++i)
   {
      if (data[i] == '\n' || (data[i] == '\r' && (i + 1 == data.size() || data[i + 1] != '\n')))
      {
         m_lines.push_back(i + 1);
      }
   }
}


void SourceLayout::record(Chunk *chunk, size_t begin, size_t end)
{
   if (chunk->Is(E_Token::CT_NEWLINE))
   {
      return;
   }
   m_tokens.push_back({
      chunk, begin, end, chunk->GetOrigLine(), chunk->GetOrigCol(),
      chunk->GetType(), chunk->IsPreproc() || chunk->Is(E_Token::CT_IGNORED)
   });
}


size_t SourceLayout::offset(size_t line, size_t column) const
{
   if (line == 0 || line > m_lines.size() || column == 0)
   {
      return(m_data.size());
   }
   size_t i   = m_lines[line - 1];
   size_t col = 1;

   while (i < m_data.size() && col < column && m_data[i] != '\n' && m_data[i] != '\r')
   {
      col = advance(col, m_data[i++], options::input_tab_size());
   }
   return(col == column ? i : m_data.size());
}


bool SourceLayout::equals(const Token &token, const char *text) const
{
   size_t i = token.begin;

   while (*text && i < token.end && m_data[i] == *text)
   {
      ++i;
      ++text;
   }
   return(!*text && i == token.end);
}


bool SourceLayout::uppercase_word(const Token &token) const
{
   bool letter = false;

   for (size_t i = token.begin; i < token.end; ++i)
   {
      int ch = m_data[i];

      if (ch >= 'A' && ch <= 'Z')
      {
         letter = true;
      }
      else if (ch != '_' && !(i > token.begin && ch >= '0' && ch <= '9'))
      {
         return(false);
      }
   }

   return(letter);
}


bool SourceLayout::contains_code(const Token &token) const
{
   static const char *const words[] =
   {
      ";", "{", "}", "if", "for", "while", "return", "class", "struct", "namespace", "try", "switch"
   };

   for (const char *word : words)
   {
      if (equals(token, word))
      {
         return(true);
      }
   }

   return(false);
}


void SourceLayout::collapse(size_t first, size_t last, bool definition)
{
   Chunk *name = m_tokens[first].chunk;
   Chunk *stop = m_tokens[last].chunk->GetNext();

   for (Chunk *pc = name->GetNext(); pc != stop;)
   {
      Chunk *next = pc->GetNext();
      Chunk::Delete(pc);
      pc = next;
   }

   name->SetType(definition ? E_Token::CT_MACRO : E_Token::CT_WORD);

   if (!definition)
   {
      // Code-bearing macros stand for statements/declarations. An invisible
      // terminator keeps their contents out of the brace/statement parser.
      Chunk semi;
      semi.SetType(E_Token::CT_SEMICOLON);
      semi.SetOrigLine(name->GetOrigLine());
      semi.SetOrigCol(0);
      semi.Text().clear();
      semi.CopyAndAddAfter(name);
   }

   for (size_t i = first; i <= last; ++i)
   {
      m_tokens[i].protected_text = true;
   }
}


void SourceLayout::protect_macros()
{
   for (size_t i = 0; i < m_tokens.size(); ++i)
   {
      // Parse configured call names as ordinary identifiers so their own
      // declarations and definitions remain distinguishable from calls.
      // Their captured token type selects special spacing during output.
      if (m_tokens[i].type == E_Token::CT_FUNC_CALL_USER)
      {
         m_tokens[i].chunk->SetType(E_Token::CT_WORD);
      }

      // Keep the directive and macro name for preprocessor frame tracking.
      if (m_tokens[i].type == E_Token::CT_PP_DEFINE && i + 1 < m_tokens.size())
      {
         size_t last = i + 1;

         while (last + 1 < m_tokens.size() && m_tokens[last + 1].protected_text)
         {
            // A following directive belongs to a different logical line.
            if (m_tokens[last + 1].type == E_Token::CT_PREPROC)
            {
               break;
            }
            ++last;
         }
         collapse(i + 1, last, true);
         i = last;
         continue;
      }

      if (m_tokens[i].protected_text || !uppercase_word(m_tokens[i]))
      {
         continue;
      }
      size_t open = i + 1;

      while (open < m_tokens.size() && comment(m_tokens[open].type))
      {
         ++open;
      }

      if (open == m_tokens.size() || !equals(m_tokens[open], "("))
      {
         continue;
      }
      size_t depth = 0;
      bool   code  = false;

      for (size_t j = open; j < m_tokens.size() && !m_tokens[j].protected_text; ++j)
      {
         if (equals(m_tokens[j], "("))
         {
            ++depth;
         }
         else if (equals(m_tokens[j], ")"))
         {
            --depth;
         }
         code |= contains_code(m_tokens[j]);

         if (depth == 0)
         {
            if (code)
            {
               m_macro_calls.emplace_back(open, j);
               collapse(i, j, false);
               i = j;
            }
            break;
         }
      }
   }
} // SourceLayout::protect_macros


bool SourceLayout::split_fields(std::deque<int> &data) const
{
   struct Edit
   {
      size_t           end;
      std::vector<int> text;
   };
   std::map<size_t, Edit>   edits;
   std::vector<Chunk *>     scopes;
   std::map<size_t, size_t> originals;
   std::vector<int>         newline;

   for (size_t i = 0; i < m_tokens.size(); ++i)
   {
      originals.emplace(m_tokens[i].begin, i);
   }

   for (size_t i = 0; i < m_data.size(); ++i)
   {
      if (m_data[i] == '\r' || m_data[i] == '\n')
      {
         newline.push_back(m_data[i]);

         if (m_data[i] == '\r' && i + 1 < m_data.size() && m_data[i + 1] == '\n')
         {
            newline.push_back('\n');
         }
         break;
      }
   }

   if (newline.empty())
   {
      newline.push_back('\n');
   }

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      if (pc->Is(E_Token::CT_BRACE_OPEN))
      {
         scopes.push_back(pc);
      }
      else if (pc->Is(E_Token::CT_BRACE_CLOSE) && !scopes.empty())
      {
         scopes.pop_back();
      }

      if (  scopes.empty() || !pc->TestFlags(PCF_VAR_1ST) || pc->IsNot(E_Token::CT_WORD)
         || pc->GetLevel() != scopes.back()->GetLevel() + 1 || pc->IsPreproc()
         || scopes.back()->TestFlags(PCF_ONE_LINER))
      {
         continue;
      }
      const E_Token scope = scopes.back()->GetParentType();

      if (scope != E_Token::CT_CLASS && scope != E_Token::CT_STRUCT && scope != E_Token::CT_UNION)
      {
         continue;
      }
      Chunk *begin = pc;

      while (begin->IsNotNullChunk() && (!begin->TestFlags(PCF_STMT_START) || begin->GetLevel() != pc->GetLevel()))
      {
         begin = begin->GetPrevNcNnl();
      }
      Chunk *end = pc->GetNextType(E_Token::CT_SEMICOLON, pc->GetLevel());

      if (begin->IsNullChunk() || end->IsNullChunk() || begin == pc || begin->GetOrigLine() != pc->GetOrigLine())
      {
         continue;
      }
      const size_t first = offset(begin->GetOrigLine(), begin->GetOrigCol());
      auto         start = originals.find(first);
      auto         last  = originals.find(offset(end->GetOrigLine(), end->GetOrigCol()));

      if (start == originals.end() || last == originals.end())
      {
         continue;
      }
      bool safe = true;

      for (size_t i = start->second; i <= last->second; ++i)
      {
         safe &= !m_tokens[i].protected_text && !comment(m_tokens[i].type);
      }

      std::vector<int> indent(m_data.begin() + m_lines[begin->GetOrigLine() - 1], m_data.begin() + first);

      for (int ch : indent)
      {
         safe &= horizontal(ch);
      }

      Chunk *declarator = pc;

      for (Chunk *part = begin; part != pc; part = part->GetNextNcNnl())
      {
         // Never duplicate a type definition or a typedef/using declaration.
         safe &= !part->IsBraceOpen() && !part->Is(E_Token::CT_TYPEDEF) && !part->Is(E_Token::CT_USING);

         if (  declarator == pc && part->GetLevel() == pc->GetLevel()
            && (part->Is(E_Token::CT_PTR_TYPE) || part->Is(E_Token::CT_BYREF)))
         {
            declarator = part;
         }
      }

      // Qualified member pointers and parenthesized declarators need a full
      // declarator grammar; leave these declarations authored as a unit.
      safe &= declarator->GetPrevNcNnl()->IsNot(E_Token::CT_DC_MEMBER);

      for (Chunk *part = declarator; part != pc; part = part->GetNextNcNnl())
      {
         safe &= part->Is(E_Token::CT_PTR_TYPE) || part->Is(E_Token::CT_BYREF) || part->Is(E_Token::CT_QUALIFIER);
      }

      size_t type_end = offset(declarator->GetOrigLine(), declarator->GetOrigCol());

      while (type_end > first && horizontal(m_data[type_end - 1]))
      {
         --type_end;
      }
      std::vector<size_t> commas;

      for (Chunk *part = pc->GetNextNcNnl(); part != end && part->IsNotNullChunk(); part = part->GetNextNcNnl())
      {
         if (part->Is(E_Token::CT_COMMA) && part->GetLevel() == pc->GetLevel())
         {
            Chunk *name = part->GetNextNcNnl();

            while (name->Is(E_Token::CT_PTR_TYPE) || name->Is(E_Token::CT_BYREF) || name->Is(E_Token::CT_QUALIFIER))
            {
               name = name->GetNextNcNnl();
            }
            safe &= name->Is(E_Token::CT_WORD) && name->TestFlags(PCF_VAR_DEF) && name->GetLevel() == pc->GetLevel();
            commas.push_back(offset(part->GetOrigLine(), part->GetOrigCol()));
         }
      }

      if (!safe || type_end <= first || commas.empty())
      {
         continue;
      }

      for (size_t comma : commas)
      {
         size_t next = comma + 1;

         while (next < m_data.size() && (horizontal(m_data[next]) || m_data[next] == '\r' || m_data[next] == '\n'))
         {
            ++next;
         }
         std::vector<int> text(1, ';');
         text.insert(text.end(), newline.begin(), newline.end());
         text.insert(text.end(), indent.begin(), indent.end());
         text.insert(text.end(), m_data.begin() + first, m_data.begin() + type_end);
         text.push_back(' ');
         edits.emplace(comma, Edit{ next, text });
      }
   }

   if (edits.empty())
   {
      return(false);
   }
   data.clear();
   size_t previous = 0;

   for (const auto &edit : edits)
   {
      data.insert(data.end(), m_data.begin() + previous, m_data.begin() + edit.first);
      data.insert(data.end(), edit.second.text.begin(), edit.second.text.end());
      previous = edit.second.end;
   }

   data.insert(data.end(), m_data.begin() + previous, m_data.end());
   return(true);
} // SourceLayout::split_fields


void SourceLayout::plan_alignment(Alignment &alignment) const
{
   struct Member
   {
      size_t begin;
      size_t column;
      size_t type_end;
      size_t last_line;
      bool   tabs;
      Chunk  *scope;
   };
   std::map<size_t, size_t> originals;
   std::vector<Chunk *>     scopes;
   std::vector<Member>      group;
   const size_t             span = options::align_func_proto_span();
   std::vector<size_t>      suffixes;
   Chunk                    *suffix_scope = nullptr;
   size_t                   suffix_line   = 0;

   auto                     flush_suffixes = [&]() {
      if (suffixes.size() > 1)
      {
         alignment.suffixes.push_back(suffixes);
      }
      suffixes.clear();
   };

   for (size_t i = 0; i < m_tokens.size(); ++i)
   {
      originals.emplace(m_tokens[i].begin, i);
   }

   std::set<size_t> added_lines;

   for (const auto &item : alignment.newlines)
   {
      added_lines.insert(item.first);
   }

   for (const auto &item : alignment.namespace_braces)
   {
      added_lines.insert(item.first);
   }

   for (const auto &item : alignment.statements)
   {
      added_lines.insert(item.first);
   }

   const std::vector<size_t> line_offsets(added_lines.begin(), added_lines.end());
   auto                      line = [&](size_t position, size_t original) {
      return(original + size_t(std::upper_bound(line_offsets.begin(), line_offsets.end(), position) - line_offsets.begin()));
   };
   auto chunk_line = [&](Chunk *pc) {
      return(line(offset(pc->GetOrigLine(), pc->GetOrigCol()), pc->GetOrigLine()));
   };

   auto flush = [&]() {
      bool   tabs    = false;
      bool   aligned = true;
      bool   wide    = false;
      size_t end     = 0;

      for (const Member &member : group)
      {
         tabs    |= member.tabs;
         aligned &= member.column == group.front().column;
         wide    |= member.column > member.type_end + options::sp_num_type_func();
         end      = std::max(end, member.type_end);
      }

      if (group.size() > 1 && (tabs || (aligned && wide)))
      {
         // A tab-aligned group uses a tab stop after the longest return type.
         // Do not append an extra space to a tab to meet the two-space rule:
         // tab alignment and two-space separation are alternative layouts.
         size_t              target = aligned ? group.front().column : advance(end, '\t', options::output_tab_size());

         std::vector<size_t> names;

         for (const Member &member : group)
         {
            alignment.members[member.begin] = target;
            names.push_back(originals.at(member.begin));
         }

         alignment.member_groups.push_back(names);
      }
      group.clear();
   };

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      if (pc->Is(E_Token::CT_BRACE_OPEN))
      {
         scopes.push_back(pc);
      }
      else if (pc->Is(E_Token::CT_BRACE_CLOSE) && !scopes.empty())
      {
         scopes.pop_back();
      }
      const E_Token parent = pc->GetParentType();

      if (  pc->Is(E_Token::CT_ACCESS)
         || (  pc->Is(E_Token::CT_PREPROC) && parent != E_Token::CT_PP_IF
            && parent != E_Token::CT_PP_ELSE && parent != E_Token::CT_PP_ENDIF))
      {
         flush_suffixes();
      }
      const bool member = !scopes.empty()
                          && (  scopes.back()->GetParentType() == E_Token::CT_CLASS
                             || scopes.back()->GetParentType() == E_Token::CT_STRUCT);
      bool operator_declaration  = false;
      bool annotated_declaration = false;

      // The parser can classify a forwarding parameter pack as a constructor
      // variable. A trailing exception annotation identifies a declaration;
      // exclude nested calls in default arguments and ordinary member fields.
      if (  member && pc->Is(E_Token::CT_FPAREN_OPEN) && parent == E_Token::CT_FUNC_CTOR_VAR
         && pc->GetLevel() == scopes.back()->GetLevel() + 1)
      {
         Chunk *close = pc->GetClosingParen();
         annotated_declaration = close->IsNotNullChunk() && close->GetNextNcNnl()->Is(E_Token::CT_NOEXCEPT);
      }

      // Conversion operators can retain ordinary parentheses and no function
      // parent. Recognize only declarations directly inside a class, excluding
      // explicit calls such as object.operator bool() in member initializers.
      if (  member && options::align_func_suffix_span() > 0 && pc->IsParenOpen() && !pc->IsPreproc()
         && pc->GetLevel() == scopes.back()->GetLevel() + 1)
      {
         for (Chunk *prev = pc->GetPrevNcNnl(); prev->IsNotNullChunk(); prev = prev->GetPrevNcNnl())
         {
            if (  prev->GetLevel() < pc->GetLevel() || prev->IsBraceOpen() || prev->IsBraceClose()
               || prev->Is(E_Token::CT_SEMICOLON) || prev->Is(E_Token::CT_ASSIGN)
               || prev->Is(E_Token::CT_PREPROC))
            {
               break;
            }

            if (prev->Is(E_Token::CT_OPERATOR))
            {
               Chunk *before = prev->GetPrevNcNnl();
               operator_declaration = before->IsNot(E_Token::CT_MEMBER)
                                      && before->IsNot(E_Token::CT_DC_MEMBER);
               break;
            }
         }
      }
      const bool class_initializer = pc->Is(E_Token::CT_FPAREN_OPEN)
                                     && (parent == E_Token::CT_FUNC_CLASS_DEF || parent == E_Token::CT_FUNC_CLASS_PROTO)
                                     && pc->GetPrevNcNnl()->TestFlags(PCF_IN_CONST_ARGS);

      if (  operator_declaration || annotated_declaration
         || (  pc->Is(E_Token::CT_FPAREN_OPEN)
            && !class_initializer
            && (  parent == E_Token::CT_FUNC_DEF || parent == E_Token::CT_FUNC_PROTO
               || parent == E_Token::CT_FUNC_CLASS_DEF || parent == E_Token::CT_FUNC_CLASS_PROTO)))
      {
         Chunk *close = pc->GetClosingParen();
         Chunk *first = pc->GetNextNcNnl();

         if (close->IsNotNullChunk() && first != close && first->IsNotNullChunk())
         {
            if (options::sp_inside_fparen_decl() != IARF_IGNORE && !pc->IsPreproc())
            {
               alignment.declaration_gaps[offset(first->GetOrigLine(), first->GetOrigCol())] = options::sp_inside_fparen_decl();
               alignment.declaration_gaps[offset(close->GetOrigLine(), close->GetOrigCol())] = options::sp_inside_fparen_decl();
            }
            std::vector<std::pair<Chunk *, Chunk *> > arguments;
            Chunk                                     *begin = first;

            for (Chunk *arg = first; arg->IsNotNullChunk() && arg != close; arg = arg->GetNext())
            {
               if (  arg->Is(E_Token::CT_COMMA) && arg->GetLevel() == pc->GetLevel() + 1
                  && !arg->TestFlags(PCF_IN_TEMPLATE))
               {
                  arguments.emplace_back(begin, arg->GetPrevNcNnl());
                  begin = arg->GetNextNcNnl();
               }
            }

            arguments.emplace_back(begin, close->GetPrevNcNnl());
            bool one_per_line = arguments.size() > 1;

            for (size_t i = 1; i < arguments.size(); ++i)
            {
               one_per_line &= arguments[i].first->GetOrigLine() > arguments[i - 1].second->GetOrigLine();
            }

            if (one_per_line)
            {
               const size_t        anchor = offset(first->GetOrigLine(), first->GetOrigCol());
               std::vector<size_t> names;

               for (const auto &argument : arguments)
               {
                  Chunk        *arg  = argument.first;
                  const size_t start = offset(arg->GetOrigLine(), arg->GetOrigCol());
                  size_t       line  = offset(arg->GetOrigLine(), 1);

                  while (line < start && horizontal(m_data[line]))
                  {
                     ++line;
                  }

                  if (line == start && start < m_data.size() && anchor < start)
                  {
                     alignment.parameters[start] = anchor;
                  }

                  if (!options::align_func_params())
                  {
                     continue;
                  }
                  bool has_name    = false;
                  bool has_default = false;

                  for (Chunk *name = arg; name->IsNotNullChunk() && name != argument.second->GetNext(); name = name->GetNext())
                  {
                     if (name->Is(E_Token::CT_ASSIGN) && name->GetLevel() == pc->GetLevel() + 1)
                     {
                        has_default = true;
                        break;
                     }

                     if (  name->TestFlags(PCF_VAR_DEF) && name->GetLevel() == pc->GetLevel() + 1
                        && name->GetOrigLine() == arg->GetOrigLine())
                     {
                        has_name = true;
                        auto original = originals.find(offset(name->GetOrigLine(), name->GetOrigCol()));

                        if (  original != originals.end() && original->second > 0
                           && !m_tokens[original->second].protected_text)
                        {
                           names.push_back(original->second);
                        }
                        break;
                     }
                  }

                  // Keep manually padded empty name columns in aligned lists.
                  if (  &argument == &arguments.back() && !has_name && !has_default
                     && arg->GetOrigLine() == close->GetOrigLine()
                     && (  argument.second->Is(E_Token::CT_TYPE) || argument.second->Is(E_Token::CT_PTR_TYPE)
                        || argument.second->Is(E_Token::CT_BYREF) || argument.second->Is(E_Token::CT_ANGLE_CLOSE)
                        || argument.second->Is(E_Token::CT_QUALIFIER)))
                  {
                     auto original = originals.find(offset(close->GetOrigLine(), close->GetOrigCol()));

                     if (  original != originals.end() && original->second > 0
                        && !m_tokens[original->second].protected_text
                        && !comment(m_tokens[original->second - 1].type))
                     {
                        alignment.declaration_gaps[m_tokens[original->second].begin] = IARF_IGNORE;
                     }
                  }
               }

               if (names.size() > 1)
               {
                  alignment.arguments.push_back(names);
               }
            }
         }

         if (member && close->IsNotNullChunk() && options::align_func_suffix_span() > 0)
         {
            Chunk *qualifier = close->GetNextNcNnl();
            auto  original   = originals.find(offset(qualifier->GetOrigLine(), qualifier->GetOrigCol()));

            if (  qualifier->Is(E_Token::CT_QUALIFIER) && qualifier->GetOrigLine() == close->GetOrigLine()
               && original != originals.end() && original->second > 0)
            {
               const Token &token    = m_tokens[original->second];
               const Token &previous = m_tokens[original->second - 1];

               if (  (equals(token, "const") || equals(token, "volatile"))
                  && !token.protected_text && token.column <= options::align_func_suffix_max_column())
               {
                  for (size_t i = previous.end; i < token.begin; ++i)
                  {
                     if (m_data[i] == '\t')
                     {
                        alignment.retained_suffixes[token.begin] = token.column;
                        break;
                     }
                  }
               }
            }

            if (  suffix_scope != scopes.back() || chunk_line(pc) <= suffix_line
               || chunk_line(pc) - suffix_line > options::align_func_suffix_span())
            {
               flush_suffixes();
            }
            suffix_scope = scopes.back();
            suffix_line  = chunk_line(close);
            bool found = false;

            for (  Chunk *suffix = close->GetNextNcNnl(); suffix->IsNotNullChunk()
                && suffix->GetOrigLine() == close->GetOrigLine(); suffix = suffix->GetNextNcNnl())
            {
               if (suffix->Is(E_Token::CT_SEMICOLON) || suffix->Is(E_Token::CT_BRACE_OPEN))
               {
                  break;
               }
               auto original = originals.find(offset(suffix->GetOrigLine(), suffix->GetOrigCol()));

               if (  suffix->Is(E_Token::CT_NOEXCEPT) && original != originals.end()
                  && original->second > 0 && !equals(m_tokens[original->second], "noexcept")
                  && !m_tokens[original->second].protected_text)
               {
                  const Token &token = m_tokens[original->second];

                  // Retain an existing tab-stop column even outside the group
                  // alignment limit. Changes inside the argument list must not
                  // add a visual tab merely by crossing an earlier tab stop.
                  if ((token.column - 1) % options::output_tab_size() == 0)
                  {
                     const Token &previous = m_tokens[original->second - 1];

                     for (size_t i = previous.end; i < token.begin; ++i)
                     {
                        if (m_data[i] == '\t')
                        {
                           alignment.retained_suffixes[token.begin] = token.column;
                           break;
                        }
                     }
                  }

                  if (token.column <= options::align_func_suffix_max_column())
                  {
                     suffixes.push_back(original->second);
                  }
                  found = true;
                  break;
               }
            }

            if (!found)
            {
               flush_suffixes();
            }
         }
      }

      if (  span == 0 || (pc->IsNot(E_Token::CT_FUNC_PROTO) && pc->IsNot(E_Token::CT_FUNC_DEF))
         || pc->GetParentType() == E_Token::CT_OPERATOR || scopes.empty()
         || (  scopes.back()->GetParentType() != E_Token::CT_CLASS
            && scopes.back()->GetParentType() != E_Token::CT_STRUCT))
      {
         continue;
      }
      Chunk *name    = step_back_over_member(pc);
      auto  original = originals.find(offset(name->GetOrigLine(), name->GetOrigCol()));

      if (original == originals.end() || original->second == 0)
      {
         continue;
      }
      const Token &token = m_tokens[original->second];
      const Token &type  = m_tokens[original->second - 1];

      if (  token.protected_text || type.protected_text || token.line != type.line
         || comment(type.type) || equals(type, "*") || equals(type, "&") || equals(type, "&&"))
      {
         flush();
         continue;
      }

      if (!group.empty() && (  group.back().scope != scopes.back()
                            || line(token.begin, token.line) <= group.back().last_line
                            || line(token.begin, token.line) - group.back().last_line > span))
      {
         flush();
      }
      bool   tabs     = false;
      size_t type_end = type.column;

      for (size_t i = type.begin; i < type.end; ++i)
      {
         type_end = advance(type_end, m_data[i], options::input_tab_size());
      }

      for (size_t i = type.end; i < token.begin; ++i)
      {
         tabs |= m_data[i] == '\t';
      }

      Chunk  *open     = pc->GetNextType(E_Token::CT_FPAREN_OPEN, pc->GetLevel());
      Chunk  *close    = open->IsNotNullChunk() ? open->GetClosingParen() : Chunk::NullChunkPtr;
      size_t last_line = close->IsNotNullChunk() ? chunk_line(close) : line(token.begin, token.line);
      group.push_back({ token.begin, token.column, type_end, last_line, tabs, scopes.back() });
   }

   flush();
   flush_suffixes();
} // SourceLayout::plan_alignment


void SourceLayout::plan_field_assignments(Alignment &alignment) const
{
   const size_t span = options::align_field_assign_span();

   if (span == 0)
   {
      return;
   }
   std::map<size_t, size_t> originals;
   std::vector<Chunk *>     scopes;
   std::vector<size_t>      group;
   Chunk                    *last_end   = Chunk::NullChunkPtr;
   Chunk                    *last_scope = Chunk::NullChunkPtr;

   for (size_t i = 0; i < m_tokens.size(); ++i)
   {
      originals.emplace(m_tokens[i].begin, i);
   }

   auto flush = [&]() {
      if (group.size() > 1)
      {
         alignment.field_assignments.push_back(group);
      }
      group.clear();
   };

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      if (pc->Is(E_Token::CT_BRACE_OPEN))
      {
         scopes.push_back(pc);
      }
      else if (pc->Is(E_Token::CT_BRACE_CLOSE) && !scopes.empty())
      {
         scopes.pop_back();
      }

      if (  scopes.empty() || !pc->TestFlags(PCF_VAR_1ST) || pc->IsNot(E_Token::CT_WORD)
         || pc->GetLevel() != scopes.back()->GetLevel() + 1 || pc->IsPreproc()
         || scopes.back()->TestFlags(PCF_ONE_LINER))
      {
         continue;
      }
      const E_Token scope = scopes.back()->GetParentType();

      if (scope != E_Token::CT_CLASS && scope != E_Token::CT_STRUCT && scope != E_Token::CT_UNION)
      {
         continue;
      }
      Chunk *begin = pc;

      while (begin->IsNotNullChunk() && (!begin->TestFlags(PCF_STMT_START) || begin->GetLevel() != pc->GetLevel()))
      {
         begin = begin->GetPrevNcNnl();
      }
      Chunk *end        = pc->GetNextType(E_Token::CT_SEMICOLON, pc->GetLevel());
      Chunk *assignment = Chunk::NullChunkPtr;
      bool  safe        = begin->IsNotNullChunk() && end->IsNotNullChunk();

      for (Chunk *part = pc->GetNextNcNnl(); safe && part != end && part->IsNotNullChunk(); part = part->GetNextNcNnl())
      {
         if (part->GetLevel() == pc->GetLevel())
         {
            safe &= part->IsNot(E_Token::CT_COMMA);

            if (part->Is(E_Token::CT_ASSIGN) && assignment->IsNullChunk())
            {
               assignment = part;
            }
         }
      }

      auto start = originals.find(offset(begin->GetOrigLine(), begin->GetOrigCol()));
      auto name  = originals.find(offset(pc->GetOrigLine(), pc->GetOrigCol()));
      auto equal = originals.find(offset(assignment->GetOrigLine(), assignment->GetOrigCol()));

      safe &= start != originals.end() && name != originals.end() && begin->GetOrigLine() == pc->GetOrigLine();

      if (safe)
      {
         for (size_t i = start->second; i <= name->second; ++i)
         {
            safe &= !m_tokens[i].protected_text && !comment(m_tokens[i].type);
         }

         for (size_t i = m_lines[begin->GetOrigLine() - 1]; i < start->first; ++i)
         {
            safe &= horizontal(m_data[i]);
         }
      }
      const bool adjacent = safe && scopes.back() == last_scope && begin->GetPrevNcNnl() == last_end
                            && begin->GetOrigLine() > last_end->GetOrigLine()
                            && begin->GetOrigLine() - last_end->GetOrigLine() <= span;

      if (!adjacent)
      {
         flush();
      }

      if (  safe && equal != originals.end() && assignment->GetOrigLine() == pc->GetOrigLine()
         && equal->second > 0 && !m_tokens[equal->second].protected_text
         && !comment(m_tokens[equal->second - 1].type))
      {
         group.push_back(equal->second);
      }
      last_scope = safe ? scopes.back() : Chunk::NullChunkPtr;
      last_end   = safe ? end : Chunk::NullChunkPtr;
   }

   flush();
} // SourceLayout::plan_field_assignments


void SourceLayout::plan_statements(Alignment &alignment) const
{
   std::map<size_t, size_t> originals;
   std::map<size_t, size_t> first_on_line;
   std::vector<Chunk *>     scopes;
   std::vector<size_t>      variables;
   Chunk                    *last_end       = Chunk::NullChunkPtr;
   Chunk                    *last_scope     = Chunk::NullChunkPtr;
   size_t                   first_statement = 0;
   unsigned                 variable_gap    = options::align_var_def_gap();
   unsigned                 variable_span   = options::align_var_def_span();

   for (size_t i = 0; i < m_tokens.size(); ++i)
   {
      originals.emplace(m_tokens[i].begin, i);
      first_on_line.emplace(m_tokens[i].line, i);
   }

   auto flush = [&]() {
      if (variables.size() > 1 && variable_span > 0)
      {
         alignment.variables.push_back({ variables, variable_gap });
      }
      variables.clear();
   };

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      if (pc->Is(E_Token::CT_BRACE_OPEN))
      {
         if (pc->GetParentType() == E_Token::CT_NAMESPACE && (options::nl_namespace_brace() & IARF_ADD))
         {
            Chunk *name    = pc->GetPrevNcNnl();
            Chunk *keyword = name;

            while (keyword->IsNotNullChunk() && keyword->IsNot(E_Token::CT_NAMESPACE) && keyword->GetLevel() == pc->GetLevel())
            {
               keyword = keyword->GetPrevNcNnl();
            }

            if (keyword->Is(E_Token::CT_NAMESPACE) && name != keyword && name->GetOrigLine() == pc->GetOrigLine())
            {
               alignment.namespace_braces[offset(pc->GetOrigLine(), pc->GetOrigCol())] = offset(keyword->GetOrigLine(), keyword->GetOrigCol());
            }
         }

         // Apply a uniform shift to a namespace function, including its body,
         // without changing the relative indentation of its statements.
         if (  options::indent_namespace() && !scopes.empty()
            && (pc->GetParentType() == E_Token::CT_FUNC_DEF || pc->GetParentType() == E_Token::CT_FUNC_CLASS_DEF))
         {
            size_t named           = 0;
            bool   namespace_scope = true;

            for (Chunk *scope : scopes)
            {
               namespace_scope &= scope->GetParentType() == E_Token::CT_NAMESPACE;
               named           += scope->GetParentType() == E_Token::CT_NAMESPACE
                                  && scope->GetPrevNcNnl()->IsNot(E_Token::CT_NAMESPACE);
            }

            if (namespace_scope && named > 0)
            {
               Chunk *begin = pc->GetPrevNcNnl();

               // Initializers can carry PCF_STMT_START at the body's level.
               // Find the function name first, then its declaration start.
               while (  begin->IsNotNullChunk()
                     && (  begin->GetLevel() != pc->GetLevel()
                        || (  begin->IsNot(E_Token::CT_FUNC_DEF)
                           && begin->IsNot(E_Token::CT_FUNC_CLASS_DEF))))
               {
                  begin = begin->GetPrevNcNnl();
               }

               while (  begin->IsNotNullChunk()
                     && (!begin->TestFlags(PCF_STMT_START) || begin->GetLevel() != pc->GetLevel()))
               {
                  begin = begin->GetPrevNcNnl();
               }
               Chunk *close   = pc->GetClosingParen();
               auto  original = originals.find(offset(begin->GetOrigLine(), begin->GetOrigCol()));

               if (  original != originals.end() && close->IsNotNullChunk()
                  && first_on_line.at(begin->GetOrigLine()) == original->second)
               {
                  const int shift = int(1 + options::indent_columns()) - int(m_tokens[original->second].column);

                  for (size_t line = begin->GetOrigLine(); line <= close->GetOrigLine(); ++line)
                  {
                     alignment.indentation[line] = shift;
                  }
               }
            }
         }
         scopes.push_back(pc);
      }
      else if (pc->Is(E_Token::CT_BRACE_CLOSE) && !scopes.empty())
      {
         scopes.pop_back();
      }

      if (  !pc->TestFlags(PCF_VAR_1ST) || scopes.empty()
         || pc->GetLevel() != scopes.back()->GetLevel() + 1
         || pc->GetFlags().test_any(PCF_IN_FOR | PCF_IN_SPAREN | PCF_IN_FCN_DEF | PCF_VAR_INLINE)
         || pc->IsPreproc())
      {
         continue;
      }
      bool local = false;

      for (Chunk *scope : scopes)
      {
         local |= scope->GetParentType() == E_Token::CT_FUNC_DEF
                  || scope->GetParentType() == E_Token::CT_FUNC_CLASS_DEF
                  || scope->GetParentType() == E_Token::CT_CPP_LAMBDA;
      }

      const E_Token  scope_type    = scopes.back()->GetParentType();
      const bool     class_fields  = scope_type == E_Token::CT_CLASS;
      const bool     struct_fields = scope_type == E_Token::CT_STRUCT || scope_type == E_Token::CT_UNION;
      const bool     fields        = class_fields || struct_fields;
      const unsigned span          = class_fields ? options::align_var_class_span()
         : struct_fields ? options::align_var_struct_span() : options::align_var_def_span();
      const unsigned gap = class_fields ? options::align_var_class_gap()
         : struct_fields ? options::align_var_struct_gap() : options::align_var_def_gap();

      if ((fields && span == 0) || (!local && !fields) || (  scopes.back()->TestFlags(PCF_ONE_LINER)
                                                          && alignment.newlines.count(offset(scopes.back()->GetOrigLine(), scopes.back()->GetOrigCol())) == 0
                                                          && alignment.newlines.count(offset(scopes.back()->GetNextNcNnl()->GetOrigLine(), scopes.back()->GetNextNcNnl()->GetOrigCol())) == 0))
      {
         continue;
      }
      Chunk *begin = pc;

      while (  begin->IsNotNullChunk()
            && (!begin->TestFlags(PCF_STMT_START) || begin->GetLevel() != pc->GetLevel()))
      {
         begin = begin->GetPrevNcNnl();
      }
      Chunk *end  = pc->GetNextType(E_Token::CT_SEMICOLON, pc->GetLevel());
      auto  name  = originals.find(offset(pc->GetOrigLine(), pc->GetOrigCol()));
      auto  start = originals.find(offset(begin->GetOrigLine(), begin->GetOrigCol()));

      if (  name == originals.end() || start == originals.end() || end->IsNullChunk()
         || begin->GetOrigLine() != pc->GetOrigLine() || start->second >= name->second
         || pc->IsNot(E_Token::CT_WORD))
      {
         flush();
         continue;
      }
      bool safe = true;

      for (size_t i = start->second; i <= name->second; ++i)
      {
         safe &= !m_tokens[i].protected_text && !comment(m_tokens[i].type);
      }

      const bool adjacent = !variables.empty() && last_scope == scopes.back()
                            && begin->GetPrevNcNnl() == last_end
                            && begin->GetOrigLine() >= last_end->GetOrigLine()
                            && begin->GetOrigLine() - last_end->GetOrigLine() <= std::max(1u, span);
      const bool own_line = first_on_line.at(begin->GetOrigLine()) == start->second
                            || alignment.newlines.count(m_tokens[start->second].begin) != 0;
      const bool split = !fields && adjacent && options::nl_split_var_def()
                         && begin->GetOrigLine() == last_end->GetOrigLine();

      if (!safe || (!own_line && !split))
      {
         flush();
         continue;
      }

      if (!adjacent)
      {
         flush();
         first_statement = m_tokens[start->second].begin;
         variable_gap    = gap;
         variable_span   = span;
      }

      if (split)
      {
         alignment.statements[m_tokens[start->second].begin] = first_statement;
      }
      variables.push_back(name->second);
      last_end   = end;
      last_scope = scopes.back();
   }

   flush();

   // Complete namespace indentation for declarations without a function body.
   // Shift a namespace-level type as a unit, retaining its internal layout.
   scopes.clear();

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      bool named           = false;
      bool namespace_scope = !scopes.empty();

      for (Chunk *scope : scopes)
      {
         namespace_scope &= scope->GetParentType() == E_Token::CT_NAMESPACE;
         named           |= scope->GetParentType() == E_Token::CT_NAMESPACE
                            && scope->GetPrevNcNnl()->IsNot(E_Token::CT_NAMESPACE);
      }

      if (options::indent_namespace() && namespace_scope && named && !pc->IsPreproc())
      {
         auto original = originals.find(offset(pc->GetOrigLine(), pc->GetOrigCol()));

         if (  original != originals.end() && first_on_line.at(pc->GetOrigLine()) == original->second
            && pc->GetLevel() == scopes.back()->GetLevel() + 1
            && !pc->IsBraceOpen() && !pc->IsBraceClose() && pc->IsNot(E_Token::CT_NAMESPACE)
            && pc->IsNot(E_Token::CT_COMMENT_MULTI) && pc->IsNot(E_Token::CT_COMMENT)
            && !m_tokens[original->second].protected_text)
         {
            const int shift = std::max(0, int(1 + options::indent_columns()) - int(m_tokens[original->second].column));

            if (shift > 0 && alignment.indentation.count(pc->GetOrigLine()) == 0)
            {
               alignment.indentation[pc->GetOrigLine()] = shift;
            }
         }

         if (pc->Is(E_Token::CT_TEMPLATE) || pc->Is(E_Token::CT_USING))
         {
            Chunk *end = pc->Is(E_Token::CT_TEMPLATE) ? pc->GetNextNcNnl()->GetClosingParen()
               : pc->GetNextType(E_Token::CT_SEMICOLON, pc->GetLevel());

            if (original != originals.end() && end->IsNotNullChunk() && first_on_line.at(pc->GetOrigLine()) == original->second)
            {
               const int shift = std::max(0, int(1 + options::indent_columns()) - int(m_tokens[original->second].column));

               if (shift > 0)
               {
                  for (size_t line = pc->GetOrigLine(); line <= end->GetOrigLine(); ++line)
                  {
                     alignment.indentation[line] = shift;
                  }
               }
            }
         }

         if (  pc->Is(E_Token::CT_BRACE_OPEN)
            && (  pc->GetParentType() == E_Token::CT_CLASS || pc->GetParentType() == E_Token::CT_STRUCT
               || pc->GetParentType() == E_Token::CT_ENUM))
         {
            Chunk *keyword = pc->GetPrevNcNnl();

            while (  keyword->IsNotNullChunk() && keyword->GetLevel() >= pc->GetLevel()
                  && keyword->IsNot(E_Token::CT_CLASS) && keyword->IsNot(E_Token::CT_STRUCT) && keyword->IsNot(E_Token::CT_ENUM)
                  && !keyword->IsBraceOpen() && !keyword->IsBraceClose() && keyword->IsNot(E_Token::CT_SEMICOLON))
            {
               keyword = keyword->GetPrevNcNnl();
            }
            Chunk *close = pc->GetClosingParen();

            if (  close->IsNotNullChunk()
               && (keyword->Is(E_Token::CT_CLASS) || keyword->Is(E_Token::CT_STRUCT) || keyword->Is(E_Token::CT_ENUM)))
            {
               const size_t row   = keyword->GetOrigLine();
               const int    shift = std::max(0, int(1 + options::indent_columns()) - int(m_tokens[first_on_line.at(row)].column));

               if (shift > 0)
               {
                  for (size_t line = row; line <= close->GetOrigLine(); ++line)
                  {
                     alignment.indentation[line] = shift;
                  }
               }
            }
         }
      }

      if (pc->Is(E_Token::CT_BRACE_OPEN))
      {
         scopes.push_back(pc);
      }
      else if (pc->Is(E_Token::CT_BRACE_CLOSE) && !scopes.empty())
      {
         scopes.pop_back();
      }
   }
} // SourceLayout::plan_statements


void SourceLayout::plan_newlines(Alignment &alignment, const std::map<size_t, size_t> &widths) const
{
   std::map<size_t, size_t>  originals;
   std::map<size_t, size_t>  first_on_line;
   std::map<size_t, Chunk *> parsed;
   std::map<size_t, Chunk *> row_anchors;
   std::vector<Chunk *>      scopes;
   std::vector<bool>         retained;
   std::set<size_t>          message_lines;
   std::set<size_t>          case_lines;

   alignment.scope_indentation.clear();

   for (size_t i = 0; i < m_tokens.size(); ++i)
   {
      originals.emplace(m_tokens[i].begin, i);
      first_on_line.emplace(m_tokens[i].line, i);
   }

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      if (pc->TestFlags(PCF_IN_OC_MSG))
      {
         message_lines.insert(pc->GetOrigLine());
      }

      if (pc->Is(E_Token::CT_CASE) || pc->Is(E_Token::CT_DEFAULT))
      {
         case_lines.insert(pc->GetOrigLine());
      }

      if (pc->Len() > 0 && !pc->IsNewline() && !pc->IsComment())
      {
         parsed.emplace(offset(pc->GetOrigLine(), pc->GetOrigCol()), pc);
      }
   }

   auto plan = [&](Chunk *pc, Chunk *base, bool comment_boundary = false) {
      auto context = row_anchors.find(base->GetOrigLine());

      if (context != row_anchors.end())
      {
         base = context->second;
      }
      const size_t begin    = offset(pc->GetOrigLine(), pc->GetOrigCol());
      auto         original = originals.find(begin);
      auto         row      = first_on_line.find(base->GetOrigLine());

      if (original == originals.end() || original->second == 0 || row == first_on_line.end())
      {
         return;
      }
      const Token &token    = m_tokens[original->second];
      const Token &previous = m_tokens[original->second - 1];
      size_t      anchor    = m_tokens[row->second].begin;
      auto        root      = parsed.find(anchor);

      if (  token.line != previous.line || token.protected_text || previous.protected_text
         || comment(token.type) || (!comment_boundary && comment(previous.type)) || root == parsed.end() || anchor >= begin
         || message_lines.count(token.line) != 0 || case_lines.count(token.line) != 0)
      {
         return;
      }
      alignment.newlines[begin] = { anchor, (int(pc->GetLevel()) - int(root->second->GetLevel())) * int(options::indent_columns()) };
   };
   Chunk *previous = Chunk::NullChunkPtr;

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      if (pc->Is(E_Token::CT_ANGLE_CLOSE) && pc->GetParentType() == E_Token::CT_TEMPLATE && !pc->IsPreproc())
      {
         Chunk *next = pc->GetNextNcNnl();
         Chunk *kind = next;

         while (kind->Is(E_Token::CT_QUALIFIER))
         {
            kind = kind->GetNextNcNnl();
         }
         iarf_e action = IARF_IGNORE;

         if (kind->Is(E_Token::CT_USING))
         {
            action = options::nl_template_using();
         }
         else if (kind->GetParentType() == E_Token::CT_FUNC_DEF || kind->GetParentType() == E_Token::CT_FUNC_CLASS_DEF)
         {
            action = options::nl_template_func_def() != IARF_IGNORE ? options::nl_template_func_def() : options::nl_template_func();
         }
         else if (kind->GetParentType() == E_Token::CT_FUNC_PROTO || kind->GetParentType() == E_Token::CT_FUNC_CLASS_PROTO)
         {
            action = options::nl_template_func_decl() != IARF_IGNORE ? options::nl_template_func_decl() : options::nl_template_func();
         }
         else if (kind->Is(E_Token::CT_CLASS) || kind->Is(E_Token::CT_STRUCT))
         {
            action = options::nl_template_class();
         }

         if (action & IARF_ADD)
         {
            Chunk *keyword = pc->GetPrevType(E_Token::CT_ANGLE_OPEN, pc->GetLevel())->GetPrevNcNnl();

            if (keyword->Is(E_Token::CT_TEMPLATE))
            {
               plan(next, keyword);
            }
         }
      }
      const bool opening     = pc->Is(E_Token::CT_BRACE_OPEN) || pc->Is(E_Token::CT_VBRACE_OPEN);
      const bool closing     = pc->Is(E_Token::CT_BRACE_CLOSE) || pc->Is(E_Token::CT_VBRACE_CLOSE);
      bool       in_function = false;
      bool       in_class    = false;

      for (Chunk *scope : scopes)
      {
         in_function |= scope->GetParentType() == E_Token::CT_FUNC_DEF || scope->GetParentType() == E_Token::CT_FUNC_CLASS_DEF;
         in_class    |= scope->GetParentType() == E_Token::CT_CLASS || scope->GetParentType() == E_Token::CT_STRUCT;
      }

      const bool function = pc->GetParentType() == E_Token::CT_FUNC_DEF || pc->GetParentType() == E_Token::CT_FUNC_CLASS_DEF;
      bool       keep     = !retained.empty() && retained.back();

      if (opening)
      {
         Chunk      *close       = pc->GetClosingParen();
         const bool single_line  = close->IsNotNullChunk() && close->GetOrigLine() == pc->GetOrigLine();
         auto       width        = widths.find(pc->GetOrigLine());
         const bool within_limit = width == widths.end() || options::nl_func_one_liner_max_columns() == 0
                                   || width->second <= options::nl_func_one_liner_max_columns();
         Chunk      *before     = pc->GetPrevNcNnl();
         const bool requirement = before->IsString("requires")
                                  || (before->IsParenClose() && before->GetOpeningParen()->GetPrevNcNnl()->IsString("requires"));
         bool       enum_expands = false;

         if (single_line && (options::nl_enum_own_lines() & IARF_ADD))
         {
            for (Chunk *item = pc->GetNextNcNnl(); item->IsNotNullChunk() && item != close; item = item->GetNextNcNnl())
            {
               if (  item->Is(E_Token::CT_BRACE_OPEN) && item->GetParentType() == E_Token::CT_ENUM
                  && item->GetNextNcNnl() != item->GetClosingParen())
               {
                  enum_expands = true;
                  break;
               }
            }
         }
         keep |= requirement;
         keep |= single_line && !enum_expands && (  (function && in_class && options::nl_func_leave_one_liners() && within_limit)
                                                 || (pc->GetParentType() == E_Token::CT_CPP_LAMBDA && options::nl_cpp_lambda_leave_one_liners()));

         if (  pc->Is(E_Token::CT_BRACE_OPEN) && pc->GetParentType() == E_Token::CT_ENUM
            && (options::nl_enum_own_lines() & IARF_ADD) && close->IsNotNullChunk())
         {
            Chunk *first = pc->GetNextNcNnl();

            if (first != close)
            {
               plan(first, pc, true);

               for (Chunk *item = first; item->IsNotNullChunk() && item != close; item = item->GetNextNcNnl())
               {
                  if (item->Is(E_Token::CT_COMMA) && item->GetLevel() == pc->GetLevel() + 1)
                  {
                     Chunk *next = item->GetNextNcNnl();

                     if (next != close)
                     {
                        plan(next, pc, true);
                     }
                  }
               }

               plan(close, pc, true);
            }
         }

         // A moved control header carries its existing body rows and closing
         // brace with it. Anchor these rows to the emitted header, so namespace
         // shifts and a second planning pass cannot add the indentation twice.
         if (  !keep && pc->Is(E_Token::CT_BRACE_OPEN) && options::nl_after_vbrace_open()
            && close->IsNotNullChunk() && close->GetOrigLine() > pc->GetOrigLine()
            && before->Is(E_Token::CT_SPAREN_CLOSE))
         {
            Chunk *header = before->GetOpeningParen()->GetPrevNcNnl();

            if (header->IsString("constexpr") && header->GetPrevNcNnl()->Is(E_Token::CT_IF))
            {
               header = header->GetPrevNcNnl();
            }
            const size_t header_position = offset(header->GetOrigLine(), header->GetOrigCol());
            auto         header_original = originals.find(header_position);
            auto         close_original  = originals.find(offset(close->GetOrigLine(), close->GetOrigCol()));

            if (  header_original != originals.end() && close_original != originals.end()
               && first_on_line.at(close->GetOrigLine()) == close_original->second
               && (  first_on_line.at(header->GetOrigLine()) == header_original->second
                  || alignment.newlines.count(header_position) != 0))
            {
               const int  baseline        = int(m_tokens[first_on_line.at(header->GetOrigLine())].column);
               const bool moved           = alignment.newlines.count(header_position) != 0;
               const bool misplaced_close = int(m_tokens[close_original->second].column) < baseline;

               for (size_t line = header->GetOrigLine() + 1; (moved || misplaced_close) && line <= close->GetOrigLine(); ++line)
               {
                  auto row = first_on_line.find(line);

                  if (row != first_on_line.end())
                  {
                     const Token &token          = m_tokens[row->second];
                     const int   original_indent = int(token.column) - baseline;
                     const int   relative        = line == close->GetOrigLine() || line == pc->GetOrigLine() ? 0
                        : moved ? original_indent : std::max(int(options::indent_columns()), original_indent);

                     if (moved || baseline + relative != int(token.column))
                     {
                        alignment.scope_indentation[line] = { header_position, relative };
                     }
                  }
               }
            }
         }

         if (function && !keep && (options::nl_fdef_brace() & IARF_ADD))
         {
            Chunk *name = pc->GetPrevNcNnl();

            while (name->IsNotNullChunk() && (  name->GetLevel() != pc->GetLevel()
                                             || (name->IsNot(E_Token::CT_FUNC_DEF) && name->IsNot(E_Token::CT_FUNC_CLASS_DEF))))
            {
               name = name->GetPrevNcNnl();
            }

            while (name->IsNotNullChunk() && (!name->TestFlags(PCF_STMT_START) || name->GetLevel() != pc->GetLevel()))
            {
               name = name->GetPrevNcNnl();
            }

            if (name->IsNotNullChunk())
            {
               row_anchors[pc->GetOrigLine()] = name;
               plan(pc, name);
            }
         }
         scopes.push_back(pc);
         retained.push_back(keep);

         if (!keep && (in_function || function))
         {
            Chunk *next = pc->GetNextNcNnl();

            if (pc->Is(E_Token::CT_VBRACE_OPEN) && options::nl_after_vbrace_open())
            {
               Chunk *base = pc->GetPrevNcNnl();

               if (base->Is(E_Token::CT_SPAREN_CLOSE))
               {
                  base = base->GetOpeningParen()->GetPrevNcNnl();
               }

               if (!next->Is(E_Token::CT_SEMICOLON))
               {
                  plan(next, base);
               }
            }
            else if (  pc->Is(E_Token::CT_BRACE_OPEN) && options::nl_after_semicolon() && next != close
                    && pc->GetParentType() != E_Token::CT_BRACED_INIT_LIST && pc->GetParentType() != E_Token::CT_FUNC_CTOR_VAR)
            {
               // Expand a body when its statements are moved to separate lines.
               plan(next, pc);
            }
         }
      }

      if (!keep && in_function && pc->Len() > 0 && !pc->IsNewline() && !pc->IsComment())
      {
         if (  options::nl_after_semicolon() && previous->Is(E_Token::CT_SEMICOLON)
            && previous->GetParentType() != E_Token::CT_FOR && !previous->TestFlags(PCF_IN_SPAREN)
            && previous->GetLevel() == previous->GetBraceLevel()
            && !pc->Is(E_Token::CT_SEMICOLON) && !pc->Is(E_Token::CT_COMMA) && !pc->Is(E_Token::CT_ANGLE_CLOSE))
         {
            plan(pc, previous);
         }
      }

      if (closing && !scopes.empty())
      {
         scopes.pop_back();
         retained.pop_back();
      }

      if (pc->Len() > 0 && !pc->IsNewline() && !pc->IsComment())
      {
         previous = pc;
      }
   }
} // SourceLayout::plan_newlines


std::map<size_t, std::vector<int> > SourceLayout::plan_endif_comments(const Alignment &alignment, std::map<size_t, std::vector<int> > &replacements) const
{
   std::map<size_t, std::vector<int> > result;
   std::vector<size_t>                 stack;
   const size_t                        threshold = options::mod_add_long_ifdef_endif_comment();

   if (threshold == 0)
   {
      return(result);
   }

   for (size_t i = 0; i < m_tokens.size(); ++i)
   {
      const Token &token = m_tokens[i];

      if (token.type == E_Token::CT_PP_IF)
      {
         stack.push_back(i);
      }
      else if (token.type == E_Token::CT_PP_ENDIF && !stack.empty())
      {
         const size_t first = stack.back();
         stack.pop_back();
         size_t       lines = token.line - m_tokens[first].line;

         for (const auto &statement : alignment.statements)
         {
            lines += statement.first > m_tokens[first].end && statement.first < token.begin;
         }

         for (const auto &brace : alignment.namespace_braces)
         {
            lines += brace.first > m_tokens[first].end && brace.first < token.begin;
         }

         for (const auto &newline : alignment.newlines)
         {
            lines += newline.first > m_tokens[first].end && newline.first < token.begin
                     && alignment.statements.count(newline.first) == 0 && alignment.namespace_braces.count(newline.first) == 0;
         }

         // Count the original block, including all #elif/#else branches.
         // Never replace an existing comment or append after other text.
         size_t end = token.end;

         while (end < m_data.size() && horizontal(m_data[end]))
         {
            ++end;
         }
         const bool existing = end < m_data.size() && m_data[end] != '\n' && m_data[end] != '\r';

         if (!existing && lines <= threshold)
         {
            continue;
         }
         std::vector<int> label;

         if (equals(m_tokens[first], "ifndef"))
         {
            const char *prefix = "ifndef ";
            label.assign(prefix, prefix + 7);
         }
         size_t previous = m_tokens[first].end;

         for (size_t j = first + 1; j < m_tokens.size(); ++j)
         {
            const Token &part = m_tokens[j];

            if (part.line > m_tokens[first].line)
            {
               // A backslash continues a logical directive; other newlines end it.
               size_t line = part.line - 1;

               if (line >= m_lines.size() || m_lines[line] < 2)
               {
                  break;
               }
               size_t pos = m_lines[line] - 1;

               if (m_data[pos] == '\n' && pos > 0 && m_data[pos - 1] == '\r')
               {
                  --pos;
               }

               if (pos == 0 || m_data[pos - 1] != '\\')
               {
                  break;
               }
            }

            if (part.type == E_Token::CT_PREPROC)
            {
               break;
            }

            if (comment(part.type) || part.type == E_Token::CT_NL_CONT || equals(part, "\\"))
            {
               if (!label.empty() && label.back() != ' ')
               {
                  label.push_back(' ');
               }
               previous = part.end;
               continue;
            }

            if (!label.empty() && previous < part.begin && label.back() != ' ')
            {
               label.push_back(' ');
            }
            label.insert(label.end(), m_data.begin() + part.begin, m_data.begin() + part.end);
            previous = part.end;
         }

         while (!label.empty() && label.back() == ' ')
         {
            label.pop_back();
         }
         const bool simplified = simplify_defined(label);

         if (existing)
         {
            // Only normalize an exact single-defined label matching its opener.
            // All other existing comments remain authored source text.
            if (simplified && i + 1 < m_tokens.size())
            {
               const Token &annotation = m_tokens[i + 1];

               if (annotation.begin == end && annotation.type == E_Token::CT_COMMENT_CPP)
               {
                  size_t begin  = annotation.begin + 2;
                  size_t finish = annotation.end;

                  while (begin < finish && horizontal(m_data[begin]))
                  {
                     ++begin;
                  }

                  while (finish > begin && horizontal(m_data[finish - 1]))
                  {
                     --finish;
                  }
                  std::vector<int> old(m_data.begin() + begin, m_data.begin() + finish);

                  if (simplify_defined(old) && old == label)
                  {
                     std::vector<int> text(m_data.begin() + annotation.begin, m_data.begin() + begin);
                     text.insert(text.end(), label.begin(), label.end());
                     text.insert(text.end(), m_data.begin() + finish, m_data.begin() + annotation.end);
                     replacements[annotation.begin] = text;
                  }
               }
            }
            continue;
         }

         if (!label.empty())
         {
            std::vector<int> text = { ' ', '/', '/', ' ' };
            text.insert(text.end(), label.begin(), label.end());
            result[token.end] = text;
         }
      }
   }

   return(result);
} // SourceLayout::plan_endif_comments


void SourceLayout::output(FILE *file) const
{
   // Match final chunks to original source boundaries. This also handles chunks
   // combined into '()' and chunks split from '>>', without a second lexer.
   std::map<size_t, size_t> starts;
   std::map<size_t, size_t> ends;
   std::set<size_t>         tab_starts;
   std::set<size_t>         operator_lines;
   std::set<size_t>         message_lines;
   std::set<size_t>         declaration_starts;
   std::map<size_t, size_t> return_spacing;
   std::set<size_t>         assignment_gaps;
   std::map<size_t, iarf_e> conditional_gaps;
   std::map<size_t, iarf_e> expression_gaps;
   std::map<size_t, size_t> initializer_starts;
   std::set<size_t>         compact_initializers;
   std::map<size_t, iarf_e> call_gaps;
   std::set<size_t>         type_names;
   std::set<size_t>         user_call_names;
   std::set<size_t>         qualifier_names;
   std::map<size_t, iarf_e> special_closes;
   std::set<size_t>         nested_closes;
   std::set<size_t>         control_closes;
   Alignment                alignment;
   std::map<size_t, size_t> emitted_columns;
   std::map<size_t, size_t> emitted_ends;
   std::map<size_t, size_t> emitted_widths;
   std::map<size_t, size_t> targets;
   std::map<size_t, size_t> suffix_targets;

   plan_alignment(alignment);
   plan_statements(alignment);
   plan_field_assignments(alignment);
   plan_newlines(alignment);
   std::map<size_t, std::vector<int> > comment_replacements;
   std::map<size_t, std::vector<int> > endif_comments;
   std::set<size_t>                    macro_gaps;

   for (const auto &call : m_macro_calls)
   {
      if (call.second > call.first + 1)
      {
         const size_t first = m_tokens[call.first + 1].begin;
         const size_t close = m_tokens[call.second].begin;
         call_gaps[first] = options::sp_inside_fparen();
         call_gaps[close] = equals(m_tokens[call.second - 1], ")")
            ? options::sp_paren_paren() : options::sp_inside_fparen();
         macro_gaps.insert(first);
         macro_gaps.insert(close);
      }
   }

   std::vector<int> line_ending;

   for (size_t i = 0; i < m_data.size(); ++i)
   {
      if (m_data[i] == '\r' || m_data[i] == '\n')
      {
         line_ending.push_back(m_data[i]);

         if (m_data[i] == '\r' && i + 1 < m_data.size() && m_data[i + 1] == '\n')
         {
            line_ending.push_back('\n');
         }
         break;
      }
   }

   if (line_ending.empty())
   {
      for (int ch : cpd.newline.get())
      {
         line_ending.push_back(ch);
      }
   }

   for (size_t i = 0; i < m_tokens.size(); ++i)
   {
      const Token &token = m_tokens[i];
      starts.emplace(token.begin, 0);
      ends.emplace(token.end, 0);

      if (token.type == E_Token::CT_TYPE)
      {
         type_names.insert(token.begin);
      }

      if (token.type == E_Token::CT_FUNC_CALL_USER)
      {
         user_call_names.insert(token.begin);
      }

      if (token.type == E_Token::CT_QUALIFIER)
      {
         qualifier_names.insert(token.begin);
      }

      if (equals(token, "operator"))
      {
         // Explicit operator calls can be mistaken for declarations. Retain
         // their layout, including aligned operator declarations on the row.
         operator_lines.insert(token.line);
      }
   }

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      if (pc->Is(E_Token::CT_SPAREN_CLOSE))
      {
         control_closes.insert(offset(pc->GetOrigLine(), pc->GetOrigCol()));
      }

      if (pc->TestFlags(PCF_IN_OC_MSG))
      {
         // A C++ call in an Objective-C receiver can be parsed as a selector.
         message_lines.insert(pc->GetOrigLine());
      }

      if (pc->Is(E_Token::CT_ASSIGN) && (options::sp_assign() & IARF_ADD))
      {
         assignment_gaps.insert(offset(pc->GetOrigLine(), pc->GetOrigCol()));
         Chunk *next = pc->GetNextNcNnl();
         assignment_gaps.insert(offset(next->GetOrigLine(), next->GetOrigCol()));
      }

      if (pc->Is(E_Token::CT_FOR_COLON))
      {
         expression_gaps[offset(pc->GetOrigLine(), pc->GetOrigCol())] = options::sp_before_for_colon();
         Chunk *next = pc->GetNextNcNnl();
         expression_gaps[offset(next->GetOrigLine(), next->GetOrigCol())] = options::sp_after_for_colon();
      }
      iarf_e expression_space = IARF_IGNORE;

      if (pc->Is(E_Token::CT_ARITH) || pc->Is(E_Token::CT_SHIFT) || pc->Is(E_Token::CT_CARET))
      {
         expression_space = options::sp_arith();

         if ((pc->IsString("+") || pc->IsString("-")) && options::sp_arith_additive() != IARF_IGNORE)
         {
            expression_space = options::sp_arith_additive();
         }
      }
      else if (pc->Is(E_Token::CT_COMPARE))
      {
         expression_space = options::sp_compare();

         if (pc->TestFlags(PCF_IN_FOR) && options::sp_for_compare() != IARF_IGNORE)
         {
            expression_space = options::sp_for_compare();
         }
         else if (pc->TestFlags(PCF_IN_SPAREN) && options::sp_sparen_compare() != IARF_IGNORE)
         {
            expression_space = options::sp_sparen_compare();
         }
      }

      if (  (pc->IsString("&") || pc->IsString("*"))
         && qualifier_names.count(offset(pc->GetPrevNcNnl()->GetOrigLine(), pc->GetPrevNcNnl()->GetOrigCol())) != 0)
      {
         expression_space = IARF_IGNORE;
      }

      if (  expression_space != IARF_IGNORE && pc->GetParentType() != E_Token::CT_OPERATOR
         && pc->GetPrevNcNnl()->IsNot(E_Token::CT_OPERATOR))
      {
         expression_gaps[offset(pc->GetOrigLine(), pc->GetOrigCol())] = expression_space;
         Chunk *next = pc->GetNextNcNnl();
         expression_gaps[offset(next->GetOrigLine(), next->GetOrigCol())] = expression_space;
      }

      if (pc->Is(E_Token::CT_SPAREN_OPEN))
      {
         Chunk         *keyword       = pc->GetPrevNcNnl();
         const E_Token control        = pc->GetParentType();
         const bool    custom_control = (control == E_Token::CT_FOR && !keyword->IsString("for"))
                                        || (control == E_Token::CT_WHILE && !keyword->IsString("while"))
                                        || (control == E_Token::CT_SWITCH && !keyword->IsString("switch"))
                                        || (control == E_Token::CT_IF && !keyword->IsString("if") && !keyword->IsString("constexpr"));
         const size_t position = offset(pc->GetOrigLine(), pc->GetOrigCol());

         if (custom_control)
         {
            alignment.declaration_gaps[position] = IARF_IGNORE;
         }
         else
         {
            expression_gaps[position] = options::sp_before_sparen();
         }
         // Control interiors are separate from function/cast interiors.
         Chunk *first = pc->GetNext();
         Chunk *close = pc->GetClosingParen();

         if (close->IsNotNullChunk() && first != close)
         {
            const bool   is_for       = control == E_Token::CT_FOR;
            const iarf_e general      = is_for ? options::sp_inside_for() : options::sp_inside_sparen();
            const iarf_e open_option  = is_for ? options::sp_inside_for_open() : options::sp_inside_sparen_open();
            const iarf_e close_option = is_for ? options::sp_inside_for_close() : options::sp_inside_sparen_close();
            const iarf_e open_space   = open_option == IARF_IGNORE ? general : open_option;
            const iarf_e close_space  = close_option == IARF_IGNORE ? general : close_option;
            call_gaps[offset(first->GetOrigLine(), first->GetOrigCol())] = open_space;
            call_gaps[offset(close->GetOrigLine(), close->GetOrigCol())] = close_space;
         }
      }

      if (pc->Is(E_Token::CT_COMMA))
      {
         Chunk *next = pc->GetNextNcNnl();

         // A trailing comma does not create padding before its closing bracket.
         if (!next->IsParenClose() && !next->IsBraceClose() && next->IsNot(E_Token::CT_ANGLE_CLOSE))
         {
            expression_gaps[offset(next->GetOrigLine(), next->GetOrigCol())] = options::sp_after_comma();
         }
      }

      if (pc->Is(E_Token::CT_SEMICOLON) && pc->GetParentType() == E_Token::CT_FOR)
      {
         Chunk *next = pc->GetNextNcNnl();
         expression_gaps[offset(next->GetOrigLine(), next->GetOrigCol())] = next->Is(E_Token::CT_SEMICOLON)
            ? options::sp_between_semi_for_empty() : next->Is(E_Token::CT_SPAREN_CLOSE)
            ? options::sp_after_semi_for_empty() : options::sp_after_semi_for();
      }

      if (pc->IsString("{") && (  pc->GetParentType() == E_Token::CT_BRACED_INIT_LIST
                               || pc->GetParentType() == E_Token::CT_FUNC_CTOR_VAR))
      {
         Chunk *close = pc->GetClosingParen();
         Chunk *first = pc->GetNext();

         if (close->IsString("}") && first != close && pc->GetOrigLine() == close->GetOrigLine())
         {
            const size_t finish  = offset(close->GetOrigLine(), close->GetOrigCol());
            const size_t start   = offset(first->GetOrigLine(), first->GetOrigCol());
            const bool   scalar  = type_names.count(offset(pc->GetPrevNcNnl()->GetOrigLine(), pc->GetPrevNcNnl()->GetOrigCol())) != 0;
            const bool   compact = start == offset(pc->GetOrigLine(), pc->GetOrigCol()) + 1
                                   && close->GetPrev()->GetOrigColEnd() == close->GetOrigCol();

            if (scalar || compact)
            {
               compact_initializers.insert(finish);

               if (scalar)
               {
                  call_gaps[start]       = options::sp_inside_paren_cast();
                  call_gaps[finish]      = options::sp_inside_paren_cast();
                  special_closes[finish] = options::sp_inside_paren_cast();
               }
            }
            else
            {
               initializer_starts[finish] = start;
            }
         }
      }

      if (pc->Is(E_Token::CT_QUESTION) || pc->Is(E_Token::CT_COND_COLON))
      {
         const bool   question = pc->Is(E_Token::CT_QUESTION);
         iarf_e       before   = question ? options::sp_cond_question_before() : options::sp_cond_colon_before();
         iarf_e       after    = question ? options::sp_cond_question_after() : options::sp_cond_colon_after();
         const iarf_e general  = question ? options::sp_cond_question() : options::sp_cond_colon();

         if (before == IARF_IGNORE)
         {
            before = general;
         }

         if (after == IARF_IGNORE)
         {
            after = general;
         }
         conditional_gaps[offset(pc->GetOrigLine(), pc->GetOrigCol())] = before;
         Chunk *next = pc->GetNextNcNnl();
         conditional_gaps[offset(next->GetOrigLine(), next->GetOrigCol())] = after;

         if (!question && pc->GetPrevNcNnl()->Is(E_Token::CT_QUESTION))
         {
            conditional_gaps[offset(pc->GetOrigLine(), pc->GetOrigCol())] = options::sp_cond_ternary_short();
         }
      }
      // Declaration boundaries remain editable when the normal indenter
      // disagrees with an included fragment or a nested namespace's layout.
      // Calls, constructor initializers and operator rows retain their rules.
      const E_Token parent  = pc->GetParentType();
      Chunk         *callee = pc->GetPrevNcNnl();
      const bool    cast    = parent == E_Token::CT_C_CAST || parent == E_Token::CT_TYPE_CAST
                              || (  parent == E_Token::CT_CPP_CAST
                                 && type_names.count(offset(callee->GetOrigLine(), callee->GetOrigCol())) != 0);

      if (  pc->Len() == 1 && pc->GetText()[0] == '('
         && (  pc->Is(E_Token::CT_FPAREN_OPEN) || pc->Is(E_Token::CT_RPAREN_OPEN)
            || (pc->Is(E_Token::CT_PAREN_OPEN) && (parent == E_Token::CT_CPP_CAST || cast)))
         && (  parent == E_Token::CT_FUNC_CALL || parent == E_Token::CT_FUNC_CALL_USER
            || parent == E_Token::CT_FUNC_CTOR_VAR || parent == E_Token::CT_CPP_CAST || cast || callee->IsString("alignas")
            || pc->GetPrevNcNnl()->TestFlags(PCF_IN_CONST_ARGS)))
      {
         Chunk *close = pc->GetClosingParen();
         Chunk *first = pc->GetNext();

         // A spaced function type in a template argument can also be labeled
         // CPP_CAST (e.g. Function<bool (const Path &)>). Keep its layout.
         Chunk      *before_type  = callee->GetPrevNcNnl();
         const bool function_type = parent == E_Token::CT_CPP_CAST && pc->TestFlags(PCF_IN_TEMPLATE)
                                    && (before_type->Is(E_Token::CT_ANGLE_OPEN) || before_type->Is(E_Token::CT_COMMA))
                                    && (  callee->IsString("void")
                                       || (  callee->GetOrigLine() == pc->GetOrigLine()
                                          && callee->GetOrigColEnd() < pc->GetOrigCol()));

         // 'requires' is not a call. Its expression parameter list uses
         // declaration spacing; grouped constraints retain their own gaps.
         // A trailing function constraint can also precede a brace, so check
         // that this requires-expression starts in an expression context.
         const bool alignment_specifier = callee->IsString("alignas");
         Chunk      *operator_word      = callee->GetPrevNcNnl();
         Chunk      *operator_type      = operator_word->GetPrevNcNnl();
         // A constrained free operator can be classified as a call. Its
         // explicit return type distinguishes it from operator+(a, b) calls.
         const bool typed_operator = callee->GetParentType() == E_Token::CT_OPERATOR
                                     && operator_word->Is(E_Token::CT_OPERATOR)
                                     && (  operator_type->Is(E_Token::CT_TYPE) || operator_type->Is(E_Token::CT_WORD)
                                        || operator_type->Is(E_Token::CT_BYREF) || operator_type->Is(E_Token::CT_PTR_TYPE)
                                        || operator_type->Is(E_Token::CT_ANGLE_CLOSE));
         const bool requires_list       = callee->IsString("requires");
         const bool requires_parameters = requires_list && close->IsNotNullChunk()
                                          && close->GetNextNcNnl()->IsBraceOpen()
                                          && (  before_type->IsNullChunk() || before_type->Is(E_Token::CT_ASSIGN)
                                             || before_type->Is(E_Token::CT_BOOL) || before_type->Is(E_Token::CT_NOT)
                                             || before_type->IsParenOpen() || before_type->IsBraceOpen()
                                             || before_type->Is(E_Token::CT_COMMA) || before_type->Is(E_Token::CT_QUESTION)
                                             || before_type->Is(E_Token::CT_COND_COLON) || before_type->Is(E_Token::CT_RETURN)
                                             || before_type->Is(E_Token::CT_CO_RETURN) || before_type->IsString("requires"));

         if ((function_type || requires_list || alignment_specifier || typed_operator) && close->IsNotNullChunk())
         {
            const iarf_e inside = (requires_parameters || alignment_specifier || typed_operator)
               ? (first == close ? options::sp_inside_fparens() : options::sp_inside_fparen_decl()) : IARF_IGNORE;
            alignment.declaration_gaps[offset(first->GetOrigLine(), first->GetOrigCol())] = inside;
            alignment.declaration_gaps[offset(close->GetOrigLine(), close->GetOrigCol())] = inside;
         }
         // Parameter-pack members may look like constructor-style variables.
         // Their exception annotation identifies a declaration, not a call.
         const bool declaration = function_type || requires_list || alignment_specifier || typed_operator
                                  || (  parent == E_Token::CT_FUNC_CTOR_VAR && close->GetNextNcNnl()->Is(E_Token::CT_NOEXCEPT)
                                     && !pc->GetPrevNcNnl()->TestFlags(PCF_IN_CONST_ARGS));

         if (!declaration && close->IsNotNullChunk() && first != close)
         {
            const bool   user_call = parent == E_Token::CT_FUNC_CALL_USER
                                     || user_call_names.count(offset(callee->GetOrigLine(), callee->GetOrigCol())) != 0;
            const iarf_e inside = cast ? options::sp_inside_paren_cast()
               : user_call ? options::sp_func_call_user_inside_fparen() : options::sp_inside_fparen();

            if (cast || user_call)
            {
               special_closes[offset(close->GetOrigLine(), close->GetOrigCol())] = inside;
            }
            call_gaps[offset(first->GetOrigLine(), first->GetOrigCol())] = inside;
            call_gaps[offset(close->GetOrigLine(), close->GetOrigCol())] = close->GetPrev()->IsParenClose()
               ? options::sp_paren_paren() : inside;
         }
         else if (!declaration && first == close)
         {
            call_gaps[offset(close->GetOrigLine(), close->GetOrigCol())] = options::sp_inside_fparens();
         }
      }

      if (  pc->Is(E_Token::CT_FPAREN_OPEN)
         && (  parent == E_Token::CT_FUNC_DEF || parent == E_Token::CT_FUNC_PROTO
            || (  (parent == E_Token::CT_FUNC_CLASS_DEF || parent == E_Token::CT_FUNC_CLASS_PROTO)
               && !pc->GetPrevNcNnl()->TestFlags(PCF_IN_CONST_ARGS))))
      {
         declaration_starts.insert(offset(pc->GetOrigLine(), pc->GetOrigCol()));
      }

      if (  (pc->Is(E_Token::CT_FUNC_DEF) || pc->Is(E_Token::CT_FUNC_PROTO))
         && parent != E_Token::CT_OPERATOR && (options::sp_type_func() & IARF_ADD))
      {
         Chunk *name = pc;

         while (name->GetPrevNcNnl()->Is(E_Token::CT_DC_MEMBER))
         {
            name = name->GetPrevNcNnl()->GetPrevNcNnl();

            if (name->Is(E_Token::CT_ANGLE_CLOSE))
            {
               // A template-id belongs to the qualification, not to the gap
               // before the name. Never insert return spacing inside '<...>'.
               name = name->GetOpeningParen()->GetPrevNcNnl();
            }
         }

         if (name->IsNotNullChunk())
         {
            const size_t begin = offset(name->GetOrigLine(), name->GetOrigCol());
            declaration_starts.insert(begin);
            return_spacing[begin] = options::sp_num_type_func();
         }
      }

      if (pc->Len() == 0 || pc->IsNewline() || pc->IsComment() || pc->TestFlags(PCF_INSERTED))
      {
         continue;
      }
      size_t                                  begin = offset(pc->GetOrigLine(), pc->GetOrigCol());
      size_t                                  end   = begin;
      bool                                    match = true;
      std::vector<std::pair<size_t, size_t> > positions;

      for (int ch : pc->GetText().get())
      {
         // Cleanup can combine tokens separated by whitespace, e.g. '( )'.
         while (end < m_data.size() && horizontal(m_data[end]) && m_data[end] != ch)
         {
            ++end;
         }

         if (end == m_data.size() || m_data[end] != ch || ch == '\n' || ch == '\r' || ch == '\t')
         {
            match = false;
            break;
         }
         positions.emplace_back(end++, pc->GetColumn() + positions.size());
      }

      if (match)
      {
         for (const auto &position : positions)
         {
            auto start  = starts.find(position.first);
            auto finish = ends.find(position.first + 1);

            if (start != starts.end())
            {
               start->second = position.second;
            }

            if (finish != ends.end())
            {
               finish->second = position.second + 1;
            }
         }

         if (  (options::align_with_tabs() && pc->TestFlags(PCF_WAS_ALIGNED))
            || (options::align_keep_tabs() && pc->GetAfterTab()))
         {
            tab_starts.insert(begin);
         }
      }
   }

   // Shared closing boundaries depend on the emitted gap before the inner
   // close, which can differ from the source after applying call spacing.
   // Keep declaration and cast interiors governed by their own options.
   for (size_t i = 1; i < m_tokens.size(); ++i)
   {
      const Token &token    = m_tokens[i];
      const Token &previous = m_tokens[i - 1];

      // A braced call argument touches the opening parenthesis, including
      // opaque macros. Comments and multiline gaps remain protected below.
      if (  equals(previous, "(") && equals(token, "{")
         && call_gaps.count(token.begin) != 0 && call_gaps.at(token.begin) != IARF_IGNORE)
      {
         call_gaps[token.begin] = IARF_REMOVE;
      }

      if (equals(previous, ")") && equals(token, ")"))
      {
         if (alignment.declaration_gaps.count(token.begin) != 0)
         {
            continue;
         }

         if (special_closes.count(token.begin) != 0)
         {
            call_gaps[token.begin] = special_closes.at(token.begin);
         }
         else if (call_gaps.count(token.begin) != 0 || control_closes.count(token.begin) != 0)
         {
            nested_closes.insert(token.begin);
            call_gaps[token.begin] = options::sp_paren_paren();
         }
         else
         {
            // Ordinary grouping can join closing brackets, but never borrows
            // the enclosing-call fallback that would insert a new inner gap.
            call_gaps[token.begin] = options::sp_paren_paren();
         }
      }
      else if (  (equals(previous, "}") && equals(token, ")"))
              || (equals(previous, ")") && equals(token, "}")))
      {
         if (compact_initializers.count(token.begin) != 0)
         {
            continue;
         }

         if (  equals(token, ")")
            && (alignment.declaration_gaps.count(token.begin) != 0 || special_closes.count(token.begin) != 0))
         {
            continue;
         }
         const bool grouping = equals(token, ")") && call_gaps.count(token.begin) == 0 && control_closes.count(token.begin) == 0;

         if (options::sp_nested_close_brace_paren() != IARF_IGNORE)
         {
            call_gaps[token.begin] = options::sp_nested_close_brace_paren();

            if (options::sp_nested_close_brace_paren() == IARF_REMOVE && !grouping)
            {
               nested_closes.insert(token.begin);
            }
         }
      }
   }

   auto render = [&](bool write) {
      emitted_columns.clear();
      emitted_ends.clear();
      emitted_widths.clear();
      size_t      previous_end          = 0;
      size_t      column                = 1;
      bool        preserve_line         = false;
      bool        previous_close_spaced = false;
      const Token *previous             = nullptr;
      auto        emit = [&](int ch) {
         if (write)
         {
            write_char(ch);
         }
         column = advance(column, ch, options::output_tab_size());
      };
      auto original = [&](size_t begin, size_t end) {
         for (size_t i = begin; i < end; ++i)
         {
            emit(m_data[i]);
         }
      };

      for (const Token &token : m_tokens)
      {
         bool         multiline_gap = false;
         bool         tabs          = false;
         size_t       old_column    = column;
         const size_t gap_column    = column;

         for (size_t i = previous_end; i < token.begin; ++i)
         {
            multiline_gap |= m_data[i] == '\n' || m_data[i] == '\r';
            tabs          |= m_data[i] == '\t';
            old_column     = advance(old_column, m_data[i], options::output_tab_size());
         }

         auto start = starts.find(token.begin);
         auto end   = ends.find(previous_end);

         if (!previous || multiline_gap)
         {
            preserve_line = start != starts.end() && start->second != 0 && start->second != token.column;
         }
         bool preserve = !previous || multiline_gap
                         || (preserve_line && declaration_starts.count(token.begin) == 0)
                         || tabs || token.protected_text
                         || previous->protected_text || comment(token.type) || comment(previous->type)
                         || operator_lines.count(token.line) != 0
                         || message_lines.count(token.line) != 0
                         || control_closes.count(previous->begin) != 0
                         || equals(*previous, ";")
                         || equals(token, ";") || equals(token, ".") || equals(*previous, ".")
                         || equals(token, "::") || equals(*previous, "::")
                         || equals(token, "->") || equals(*previous, "->")
                         || equals(token, "*") || equals(*previous, "*")
                         || equals(token, "&") || equals(*previous, "&")
                         || equals(token, "&&") || equals(*previous, "&&")
                         || equals(token, "++") || equals(*previous, "++")
                         || equals(token, "--") || equals(*previous, "--")
                         || equals(token, "{") || equals(token, "}") || equals(token, "else")
                         || equals(*previous, "else")
                         || equals(*previous, "not")
                         || equals(token, "requires") || equals(*previous, "requires")
                         || (equals(token, "(") && equals(*previous, "("))
                         || (token.type == E_Token::CT_NOEXCEPT && !equals(token, "noexcept"))
                         || equals(token, "...") || equals(*previous, "{")
                         || equals(*previous, "}") || equals(*previous, "...")
                         || start == starts.end() || end == ends.end()
                         || start->second == 0 || end->second == 0
                         || start->second < end->second;

         auto   declaration_gap = alignment.declaration_gaps.find(token.begin);
         auto   call_gap        = call_gaps.find(token.begin);
         auto   conditional_gap = conditional_gaps.find(token.begin);
         auto   expression_gap  = expression_gaps.find(token.begin);
         iarf_e call_space      = call_gap == call_gaps.end() ? IARF_IGNORE : call_gap->second;

         if (nested_closes.count(token.begin) != 0 && !previous_close_spaced)
         {
            call_space = options::sp_inside_fparen();
         }
         auto parameter = alignment.parameters.find(token.begin);
         auto member    = alignment.members.find(token.begin);
         auto retained  = alignment.retained_suffixes.find(token.begin);
         auto target    = targets.find(token.begin);
         auto anchor    = parameter == alignment.parameters.end() ? emitted_columns.end() : emitted_columns.find(parameter->second);

         auto statement       = alignment.statements.find(token.begin);
         auto newline         = alignment.newlines.find(token.begin);
         auto namespace_brace = alignment.namespace_braces.find(token.begin);
         auto indentation     = alignment.indentation.find(token.line);
         auto scope_indent    = alignment.scope_indentation.find(token.line);

         if (  !multiline_gap && newline != alignment.newlines.end() && !token.protected_text
            && emitted_columns.count(newline->second.anchor) != 0)
         {
            for (int ch : line_ending)
            {
               emit(ch);
            }

            const size_t indent = size_t(std::max(1, int(emitted_columns.at(newline->second.anchor)) + newline->second.indent));

            while (advance(column, '\t', options::output_tab_size()) <= indent)
            {
               emit('\t');
            }

            while (column < indent)
            {
               emit(' ');
            }
         }
         else if (  !multiline_gap && namespace_brace != alignment.namespace_braces.end() && !token.protected_text
                 && previous && !previous->protected_text
                 && emitted_columns.count(namespace_brace->second) != 0)
         {
            for (int ch : line_ending)
            {
               emit(ch);
            }

            const size_t indent = emitted_columns.at(namespace_brace->second);

            while (advance(column, '\t', options::output_tab_size()) <= indent)
            {
               emit('\t');
            }

            while (column < indent)
            {
               emit(' ');
            }
         }
         else if (  !multiline_gap && statement != alignment.statements.end() && !token.protected_text
                 && emitted_columns.count(statement->second) != 0)
         {
            for (int ch : line_ending)
            {
               emit(ch);
            }

            const size_t indent = emitted_columns.at(statement->second);

            while (advance(column, '\t', options::output_tab_size()) <= indent)
            {
               emit('\t');
            }

            while (column < indent)
            {
               emit(' ');
            }
         }
         else if (multiline_gap && anchor != emitted_columns.end() && !token.protected_text)
         {
            size_t indentation = token.begin;

            while (indentation > previous_end && horizontal(m_data[indentation - 1]))
            {
               --indentation;
            }
            original(previous_end, indentation);

            while (advance(column, '\t', options::output_tab_size()) <= anchor->second)
            {
               emit('\t');
            }

            while (column < anchor->second)
            {
               emit(' ');
            }
         }
         else if (  (!previous || multiline_gap) && !token.protected_text
                 && (  (indentation != alignment.indentation.end() && indentation->second != 0)
                    || (scope_indent != alignment.scope_indentation.end() && emitted_columns.count(scope_indent->second.anchor) != 0))
                 && token.type != E_Token::CT_COMMENT_MULTI && token.type != E_Token::CT_COMMENT)
         {
            size_t begin = token.begin;

            while (begin > previous_end && horizontal(m_data[begin - 1]))
            {
               --begin;
            }
            original(previous_end, begin);
            const size_t indent = scope_indent != alignment.scope_indentation.end() && emitted_columns.count(scope_indent->second.anchor) != 0
               ? size_t(std::max(1, int(emitted_columns.at(scope_indent->second.anchor)) + scope_indent->second.indent))
               : size_t(std::max(1, int(old_column) + indentation->second));

            while (advance(column, '\t', options::output_tab_size()) <= indent)
            {
               emit('\t');
            }

            while (column < indent)
            {
               emit(' ');
            }
         }
         else if (  !multiline_gap && assignment_gaps.count(token.begin) != 0
                 && targets.count(token.begin) == 0
                 && previous && !token.protected_text && !previous->protected_text
                 && !comment(token.type) && !comment(previous->type)
                 && operator_lines.count(token.line) == 0 && message_lines.count(token.line) == 0
                 && old_column == column)
         {
            emit(' ');
         }
         else if (  !multiline_gap && expression_gap != expression_gaps.end() && expression_gap->second != IARF_IGNORE
                 && previous && !token.protected_text && !previous->protected_text
                 && !comment(token.type) && !comment(previous->type)
                 && message_lines.count(token.line) == 0)
         {
            if (expression_gap->second == IARF_ADD && old_column > column)
            {
               original(previous_end, token.begin);
            }
            else if (expression_gap->second != IARF_REMOVE)
            {
               emit(' ');
            }
         }
         else if (  !multiline_gap && conditional_gap != conditional_gaps.end() && conditional_gap->second != IARF_IGNORE
                 && previous && !token.protected_text && !previous->protected_text
                 && !comment(token.type) && !comment(previous->type)
                 && message_lines.count(token.line) == 0)
         {
            if (conditional_gap->second == IARF_ADD && old_column > column)
            {
               original(previous_end, token.begin);
            }
            else if (conditional_gap->second != IARF_REMOVE)
            {
               emit(' ');
            }
         }
         else if (  !multiline_gap && call_gap != call_gaps.end() && call_space != IARF_IGNORE
                 && previous && ((!token.protected_text && !previous->protected_text) || macro_gaps.count(token.begin) != 0)
                 && !comment(token.type) && !comment(previous->type)
                 && message_lines.count(token.line) == 0)
         {
            if (call_space == IARF_ADD && old_column > column)
            {
               original(previous_end, token.begin);
            }
            else if (call_space != IARF_REMOVE)
            {
               emit(' ');
            }
         }
         else if (  !multiline_gap && declaration_gap != alignment.declaration_gaps.end()
                 && previous && !token.protected_text && !previous->protected_text
                 && !comment(token.type) && !comment(previous->type)
                 && operator_lines.count(token.line) == 0 && message_lines.count(token.line) == 0)
         {
            if (  declaration_gap->second == IARF_IGNORE
               || (declaration_gap->second == IARF_ADD && old_column > column))
            {
               original(previous_end, token.begin);
            }
            else if (declaration_gap->second != IARF_REMOVE)
            {
               emit(' ');
            }
         }
         else if (!multiline_gap && suffix_targets.count(token.begin) != 0 && !token.protected_text)
         {
            const size_t suffix_target = suffix_targets.at(token.begin);

            if (old_column == suffix_target)
            {
               original(previous_end, token.begin);
            }
            else
            {
               // Suffix targets are tab stops. Never generate space padding,
               // and allow a zero gap when a suffix already meets its peers.
               while (column < suffix_target)
               {
                  emit('\t');
               }
            }
         }
         else if (!multiline_gap && target != targets.end() && !token.protected_text && target->second > column)
         {
            if (old_column == target->second)
            {
               original(previous_end, token.begin);
            }
            else
            {
               while (options::align_with_tabs() && advance(column, '\t', options::output_tab_size()) <= target->second)
               {
                  emit('\t');
               }

               while (column < target->second)
               {
                  emit(' ');
               }
            }
         }
         else if (retained != alignment.retained_suffixes.end() && !token.protected_text && retained->second > column)
         {
            if (old_column == retained->second)
            {
               original(previous_end, token.begin);
            }
            else
            {
               while (advance(column, '\t', options::output_tab_size()) <= retained->second)
               {
                  emit('\t');
               }

               while (column < retained->second)
               {
                  emit(' ');
               }
            }
         }
         else if (member != alignment.members.end() && !token.protected_text && member->second >= column)
         {
            // Earlier spacing on the same row can reach the old name column.
            // Retain a nonempty gap so adjacent identifiers cannot merge.
            if (old_column == member->second || (member->second == column && old_column > column))
            {
               original(previous_end, token.begin);
            }
            else
            {
               while (advance(column, '\t', options::output_tab_size()) <= member->second)
               {
                  emit('\t');
               }

               while (column < member->second)
               {
                  emit(' ');
               }
            }
         }
         else if (preserve)
         {
            original(previous_end, token.begin);
         }
         else
         {
            size_t width      = start->second - end->second;
            auto   return_gap = return_spacing.find(token.begin);

            if (return_gap != return_spacing.end())
            {
               width = return_gap->second;

               if (options::sp_type_func() != IARF_FORCE)
               {
                  width = std::max(width, old_column - column);
               }
            }
            size_t target = column + width;

            if (old_column - column == width)
            {
               original(previous_end, token.begin);
            }
            tabs |= tab_starts.count(token.begin) != 0;

            // Retain existing tab-based alignment when its visual width changes.
            while (tabs && advance(column, '\t', options::output_tab_size()) <= target)
            {
               emit('\t');
            }

            while (column < target)
            {
               emit(' ');
            }
         }
         previous_close_spaced = (equals(token, ")") || equals(token, "}"))
                                 && (multiline_gap || column > gap_column);
         emitted_columns[token.begin] = column;
         auto replacement = comment_replacements.find(token.begin);

         if (replacement == comment_replacements.end())
         {
            original(token.begin, token.end);
         }
         else
         {
            for (int ch : replacement->second)
            {
               emit(ch);
            }
         }
         auto endif_comment = endif_comments.find(token.end);

         if (endif_comment != endif_comments.end())
         {
            for (int ch : endif_comment->second)
            {
               emit(ch);
            }
         }
         emitted_ends[token.end]    = column;
         emitted_widths[token.line] = std::max(emitted_widths[token.line], column - 1);
         previous                   = &token;
         previous_end               = token.end;
      }

      original(previous_end, m_data.size());

      if (  (m_data.empty() || (m_data.back() != '\n' && m_data.back() != '\r'))
         && (options::nl_end_of_file() & IARF_ADD))
      {
         for (int ch : cpd.newline.get())
         {
            emit(ch);
         }
      }
   };

   // A compact call at the end of an initializer can acquire an enclosing
   // closing space. Mirror that emitted space after the opening brace.
   if (!initializer_starts.empty())
   {
      render(false);

      for (size_t i = 1; i < m_tokens.size(); ++i)
      {
         const Token &token = m_tokens[i];
         auto        first  = initializer_starts.find(token.begin);

         if (  first != initializer_starts.end() && nested_closes.count(token.begin) != 0
            && emitted_columns.at(token.begin) > emitted_ends.at(m_tokens[i - 1].end))
         {
            call_gaps[first->second] = IARF_ADD;
         }
      }
   }

   for (unsigned measurement = 0; measurement < 2; ++measurement)
   {
      // A template prefix on a member's row may grow when assignment spacing
      // changes. Align the whole group against its emitted columns in this pass,
      // before measuring argument names or trailing suffixes.
      if (!alignment.member_groups.empty())
      {
         render(false);

         for (const auto &group : alignment.member_groups)
         {
            bool   aligned = true;
            size_t target  = 0;

            for (size_t index : group)
            {
               const size_t current = emitted_columns.at(m_tokens[index].begin);
               target   = std::max(target, current);
               aligned &= current == emitted_columns.at(m_tokens[group.front()].begin);
            }

            if (!aligned)
            {
               target = advance(target - 1, '\t', options::output_tab_size());

               for (size_t index : group)
               {
                  alignment.members[m_tokens[index].begin] = target;
               }
            }
         }
      }

      // Measure actual output columns before adding argument-name padding. A
      // second measurement accounts for that padding when aligning suffixes.
      if (!alignment.arguments.empty() || !alignment.variables.empty())
      {
         render(false);

         auto padding_index = [&](size_t index) {
            size_t padding = index;

            while (  padding > 0 && m_tokens[padding - 1].end == m_tokens[padding].begin
                  && (  equals(m_tokens[padding - 1], "*") || equals(m_tokens[padding - 1], "&")
                     || equals(m_tokens[padding - 1], "&&") || equals(m_tokens[padding - 1], "...")))
            {
               --padding;
            }
            return(padding);
         };

         for (const auto &group : alignment.variables)
         {
            size_t target  = 0;
            bool   aligned = true;

            for (size_t index : group.names)
            {
               const size_t padding  = padding_index(index);
               const size_t attached = emitted_columns[m_tokens[index].begin] - emitted_columns[m_tokens[padding].begin];
               target   = std::max(target, emitted_columns[m_tokens[index].begin]);
               target   = std::max(target, emitted_ends[m_tokens[padding - 1].end] + std::max(1u, group.gap) + attached);
               aligned &= emitted_columns[m_tokens[index].begin] == emitted_columns[m_tokens[group.names.front()].begin];
            }

            if (!aligned && options::align_with_tabs())
            {
               target = advance(target - 1, '\t', options::output_tab_size());
            }

            for (size_t index : group.names)
            {
               const size_t padding  = padding_index(index);
               const size_t attached = emitted_columns[m_tokens[index].begin] - emitted_columns[m_tokens[padding].begin];
               targets[m_tokens[padding].begin] = target - attached;
            }
         }

         for (const auto &group : alignment.arguments)
         {
            size_t target = 0;

            for (size_t index : group)
            {
               const size_t padding  = padding_index(index);
               const size_t attached = emitted_columns[m_tokens[index].begin] - emitted_columns[m_tokens[padding].begin];
               target = std::max(target, emitted_columns[m_tokens[index].begin]);

               if (padding > 0)
               {
                  target = std::max(target, emitted_ends[m_tokens[padding - 1].end] + 1 + attached);
               }
            }

            for (size_t index : group)
            {
               const size_t padding  = padding_index(index);
               const size_t attached = emitted_columns[m_tokens[index].begin] - emitted_columns[m_tokens[padding].begin];
               targets[m_tokens[padding].begin] = target - attached;
            }
         }
      }

      if (!alignment.field_assignments.empty())
      {
         render(false);

         for (const auto &group : alignment.field_assignments)
         {
            size_t target  = 0;
            bool   aligned = true;

            for (size_t index : group)
            {
               target   = std::max(target, emitted_columns[m_tokens[index].begin]);
               target   = std::max(target, emitted_ends[m_tokens[index - 1].end] + 1);
               aligned &= emitted_columns[m_tokens[index].begin] == emitted_columns[m_tokens[group.front()].begin];
            }

            if (!aligned && options::align_with_tabs())
            {
               target = advance(target - 1, '\t', options::output_tab_size());
            }

            for (size_t index : group)
            {
               targets[m_tokens[index].begin] = target;
            }
         }
      }

      if (!alignment.suffixes.empty())
      {
         render(false);

         for (const auto &group : alignment.suffixes)
         {
            size_t target   = 0;
            size_t required = 0;
            bool   aligned  = true;

            for (size_t index : group)
            {
               target   = std::max(target, m_tokens[index].column);
               required = std::max(required, emitted_ends[m_tokens[index - 1].end]);
               aligned &= emitted_columns[m_tokens[index].begin] == emitted_columns[m_tokens[group.front()].begin];
            }

            if (aligned)
            {
               continue;
            }
            target = advance(std::max(required, target) - 1, '\t', options::output_tab_size());

            if (target <= options::align_func_suffix_max_column())
            {
               for (size_t index : group)
               {
                  suffix_targets[m_tokens[index].begin] = target;
               }
            }
         }
      }

      if (measurement == 0)
      {
         render(false);
         plan_newlines(alignment, emitted_widths);
         alignment.members.clear();
         alignment.member_groups.clear();
         alignment.arguments.clear();
         alignment.suffixes.clear();
         targets.clear();
         suffix_targets.clear();
         plan_alignment(alignment);
         alignment.variables.clear();
         alignment.statements.clear();
         plan_statements(alignment);
      }
   }

   endif_comments = plan_endif_comments(alignment, comment_replacements);
   cpd.fout       = file;

   if (cpd.hasBom)
   {
      write_bom();
   }
   render(true);
} // SourceLayout::output
