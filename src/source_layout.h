/**
 * @file source_layout.h
 * Preserve source layout while applying horizontal spacing and alignment.
 * @license GPL v2+
 */
#ifndef SOURCE_LAYOUT_H_INCLUDED
#define SOURCE_LAYOUT_H_INCLUDED

#include "option.h"
#include "token_enum.h"
#include <cstdio>
#include <deque>
#include <map>
#include <vector>

class Chunk;

class SourceLayout
{
public:
   explicit SourceLayout(const std::deque<int> &data);
   void record(Chunk *chunk, size_t begin, size_t end);
   void protect_macros();
   bool split_fields(std::deque<int> &data) const;
   void output(FILE *file) const;

private:
   struct Token
   {
      Chunk   *chunk;
      size_t  begin;
      size_t  end;
      size_t  line;
      size_t  column;
      E_Token type;
      bool    protected_text;
   };

   struct Alignment
   {
      struct Newline
      {
         size_t anchor;
         int    indent;
      };
      struct VariableGroup
      {
         std::vector<size_t> names;
         unsigned            gap;
      };
      std::map<size_t, uncrustify::iarf_e> declaration_gaps;
      std::map<size_t, size_t>             parameters;
      std::map<size_t, size_t>             members;
      std::map<size_t, size_t>             retained_suffixes;
      std::map<size_t, size_t>             statements;
      std::map<size_t, size_t>             namespace_braces;
      std::map<size_t, Newline>            newlines;
      std::map<size_t, Newline>            scope_indentation;
      std::map<size_t, int>                indentation;
      std::vector<VariableGroup>           variables;
      std::vector<std::vector<size_t> >    field_assignments;
      std::vector<std::vector<size_t> >    member_groups;
      std::vector<std::vector<size_t> >    arguments;
      std::vector<std::vector<size_t> >    suffixes;
   };

   size_t offset(size_t line, size_t column) const;
   bool equals(const Token &token, const char *text) const;
   bool uppercase_word(const Token &token) const;
   bool contains_code(const Token &token) const;
   void collapse(size_t first, size_t last, bool definition);
   void plan_alignment(Alignment &alignment) const;
   void plan_field_assignments(Alignment &alignment) const;
   void plan_statements(Alignment &alignment) const;
   void plan_newlines(Alignment &alignment, const std::map<size_t, size_t> &widths = {}) const;
   std::map<size_t, std::vector<int> > plan_endif_comments(const Alignment &alignment, std::map<size_t, std::vector<int> > &replacements) const;

   const std::deque<int>                   &m_data;
   std::vector<size_t>                     m_lines;
   std::vector<Token>                      m_tokens;
   std::vector<std::pair<size_t, size_t> > m_macro_calls;
};

#endif /* SOURCE_LAYOUT_H_INCLUDED */
