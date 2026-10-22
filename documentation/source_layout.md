# Preserving an existing layout

`preserve_source_layout = true` applies the parser's horizontal spacing and
alignment decisions to the original source. It preserves existing line breaks,
indentation (except namespace declarations and one-argument-per-line lists), comment contents and gaps,
literal contents, preprocessor directives,
brace/else spacing, parameter-pack spacing, and equivalent tabs. If the normal
indenter would move a row's first token, ordinary gaps on that row are retained.
Function declaration boundaries still follow their configured spacing: the
return-type/name gap and the gap before `(` remain editable, while tabs and
protected boundaries retain their layout. Qualified names such as
`uint  Compiler::Width ()` use the same return-type spacing. This supports implementation fragments included inside namespaces and declarations
with extra leading padding for annotations.

Existing gaps containing tabs and gaps beside member access, pointer/reference
tokens, increment/decrement, after `else`, and before and after semicolons are retained.
Explicit binary-expression spacing and `for` header options described below
can edit their own boundaries, while declaration pointer/reference and unary
operator attachment remains unchanged.
Rows containing `operator` also retain their horizontal layout, except for the
class-member suffix alignment described below: explicit operator calls can
otherwise be misidentified as declarations and aligned.
Objective-C message rows retain their layout to avoid misclassifying C++ receiver
expressions as selectors. Spacing after the C++ alternative token `not` and
around `requires` is retained. Parameter lists in requires-expressions follow
`sp_inside_fparen_decl`, so `requires ( From from )` becomes
`requires (From from)` with `remove`. Empty parameter lists use
`sp_inside_fparens`. Grouped constraint boundaries retain their existing gaps,
including trailing constraints before a function body. Calls within requirement
bodies still use call spacing. Comments and line breaks remain protected.
Alignment specifiers (`alignas(...)`) use declaration interiors too, including
when the parser classifies the specifier as a macro call.

The mode captures token ranges during native tokenization, before cleanup. Macro
definitions and uppercase function-like macro calls containing statements or
declarations are hidden from subsequent parsing. Their source is emitted intact.
Argument-list macros such as `DEBUG_ONLY(, Path filename)` remain parseable.
All of this happens inside Uncrustify; no preprocessing script is required.

The output edits gaps between original tokens. Token-changing options have no
effect on output in this mode. Supported newline options are applied explicitly:
`nl_end_of_file` may add a final newline and `nl_split_var_def` may split
adjacent local declaration statements. Function braces, control bodies and
statements can also receive newlines as described under "Statement newlines
and compact member width" below. `nl_namespace_brace = add` moves named
namespace opening braces to a new line at the namespace keyword's indentation;
anonymous namespaces retain their brace layout. `mod_add_long_ifdef_endif_comment`
may add missing conditional-block labels or shorten matching single-defined
labels. Existing line-ending sequences are
preserved. HTML tracking still uses the normal output path. The default is false,
so ordinary formatting is unchanged.

Declaration and definition parameters align with the first argument's actual
output column only when every argument starts on a separate line. The first
argument may share a line with the function name. If any line has multiple
arguments, the entire list retains its existing alignment. Nested commas in
templates, calls, and default expressions do not count as argument separators.

With `align_func_params = true`, parameter names in these lists align too. The
writer measures actual output columns after indentation and return-type/name
spacing, so changing indentation cannot leave tab-separated names misaligned.
Attached pointer/reference tokens remain attached to their parameter names.
Identifiers align with identifiers; an attached `*` or `&` occupies the column
before its name rather than sharing the plain name's column.
Unnamed parameters and nested declarators retain their internal layout.
For an unnamed final parameter on its own argument row, `align_func_params = true`
preserves padding before the closing `)` as an empty name slot.
Existing padding is retained, including with compact declaration
interiors enabled. This exception requires a list with one argument per line;
single-line lists, mixed lists, named final parameters, and separate closing
parenthesis lines continue to follow their existing rules.

`sp_inside_fparen_decl` controls spaces inside nonempty declaration and
definition parentheses separately from calls. Its default, `ignore`, falls back
to `sp_inside_fparen`; empty lists still use `sp_inside_fparens`. Set it to
`remove` to produce `make_error_code (Result e)` while retaining call spacing.
In preservation mode this explicit boundary rule also applies to retained rows
and inner tabs. Boundary comments, line breaks, protected macros, operator rows,
and Objective-C messages keep their layout.

Constructor/destructor declaration spacing uses `sp_func_class_paren` and
`sp_func_class_paren_empty`. In preservation mode, destructor calls, bitwise
complements of calls, and constructor initializer calls use call spacing even
when their names retain a constructor classification. Initializer arguments
are excluded from declaration-interior and parameter alignment rules.

# Return-type spacing

`sp_num_type_func` controls the number of spaces used by `sp_type_func = add` or
`force`. Its default is 1. For exactly two spaces on recognized function
declarations and definitions, disable prototype alignment and use:

```ini
sp_type_func = force
sp_num_type_func = 2
align_func_proto_span = 0
```

Use `sp_type_func = add` to retain wider existing gaps. Without preservation
mode, ordinary prototype alignment uses `align_func_proto_gap` and a nonzero
`align_func_proto_span` and may widen gaps.

In preservation mode, a nonzero `align_func_proto_span` groups adjacent direct
class and struct member functions only. Free functions use the return-type
spacing rule without group alignment. A group using two-space separation keeps
that style. Already aligned names retain their exact original gaps, including
mixed tabs and spaces. A name touching a template return type may retain its
zero-width gap when it already meets the group column. If a group contains
tabs but its names are inconsistent,
the names align at the first tab stop after the longest return type. Tab alignment
does not require an additional space to meet `sp_num_type_func`: these are
alternative layouts. Multiline declarations can belong to the same group when
the following declaration starts within the configured span after the closing
parameter parenthesis.

Pointer/reference-specific spacing options and protected source ranges
described above take precedence, except for the explicit parameter-name
alignment described above. Retained rows still apply return-type/name spacing
and the configured space before declaration parentheses.

# Class-member suffix macros

In preservation mode, `align_func_suffix_span` enables alignment of configured
exception/const macros such as `__NE_OV`, `C_NE_OV`, and `__Th___` after class or
struct member parameter lists. Its default is zero (disabled). A value of 4
groups nearby declarations, including up to four lines from the preceding
declaration's closing parenthesis. Conversion and overloaded operators join the
same groups as ordinary member functions. Conditional directives (`#if`, `#elif`,
`#else`, and `#endif`) do not interrupt nearby groups within that span. Access
labels, other preprocessor directives, scope changes, and member functions without
a suffix end a group. Free functions and declarations inside function bodies
are excluded.

Forwarding parameter packs followed by exception annotations are included even
when the parser classifies them as constructor-style variables. They do not
split a group between neighboring ordinary or templated member functions.

`align_func_suffix_max_column` defaults to 100 and counts visual columns from
the line start, including indentation and expanded tabs. Existing suffix columns
beyond this limit retain their gaps. Already aligned suffixes and standalone
members keep their existing gaps, including no gap in `)__NE___`. When alignment
is needed, the writer starts with the widest existing suffix column, accounts
for return-type, parameter, and name alignment, and rounds up to a tab stop.
New suffix padding uses tabs only; no separating space or tab is mandatory.
If rounding would exceed the limit, the group's existing suffix gaps are
retained. No line breaks are added. Existing gaps before configured suffix
macros are preserved unless this alignment rule changes them; ordinary C++
`noexcept` spacing still follows the normal spacing options.

Within the same scope and column limit, native `const`/`volatile` qualifiers
that already have tab-separated padding retain their original columns when
earlier spacing changes, for example when `GetDevice()` becomes `GetDevice ()`.
Ordinary single-space qualifiers are not given column alignment.

Existing tab-stop columns of configured suffix macros are retained when earlier
argument spacing changes, including beyond the group-alignment limit. This
preserves manual alignment with distant columns without widening a group or
adding spaces before macros. It avoids shifting a suffix by a full tab when a
one-column change inside the argument list moves `)` across a tab stop.

# Local declarations, namespace functions and assignments

In preservation mode, `nl_split_var_def = true` separates adjacent local
variable declaration statements such as `String log; bool success=false;`.
It uses parsed declaration boundaries, retaining `for` headers, inline bodies,
comments, macro bodies and multi-declarator statements. Inserted line endings
match the source file. The following declaration starts at the first
declaration's actual output indentation.

A nonzero `align_var_def_span` aligns names in consecutive local declaration
statements within that line span. Names align using actual output columns after
splitting and indentation. Attached pointer/reference tokens remain attached,
and existing aligned groups keep their columns. Other groups use tab stops
when `align_with_tabs = true`. `align_var_def_gap` sets the minimum separation.

With `indent_namespace = true`, direct function definitions inside named
namespaces use one indentation level, including functions in a nested anonymous
namespace. Nested named namespaces do not add levels in preservation mode.
The whole definition shifts uniformly, retaining relative body indentation.
Preprocessor directives and literal contents keep their original layout.
Fragments without an explicit named namespace keep their indentation.

With `sp_assign = add` or `force`, parsed assignment operators receive a
minimum one-character gap on both sides even on retained rows. Existing spaces
and tabs remain. Protected macros, comments, directives and operator rows retain
their layout. Comparisons and assignment operators inside literals are untouched.

Parsed ternary `?` and `:` boundaries use `sp_cond_question` and `sp_cond_colon`,
including retained rows and boundaries next to pointer/address operators.
The before/after overrides and `sp_cond_ternary_short` remain independent.
`add` retains existing wider spacing and tabs, while adding a missing separator.
Comments, directives, line breaks and opaque macro bodies keep their layout.

`sp_arith` controls parsed binary arithmetic, shift and bitwise operators;
`sp_arith_additive` can override `+` and `-`. `sp_compare` controls parsed
comparison operators, with `sp_for_compare` and `sp_sparen_compare` overrides
inside control headers. These explicit rules apply even on retained rows and
inside overloaded operator bodies. Overloaded operator names, unary operators,
declaration pointer/reference tokens and template delimiters are excluded.
With `add`, existing wider spaces and tabs are retained.
The original classification of configured qualifiers is retained for this
decision: an `&` or `*` following a qualifier such as `OUT` or `INOUT` stays
attached to its argument, including at the start of a continuation line where
the normal parser can label it as binary arithmetic.

`sp_before_sparen` applies before ordinary control parentheses. Configured
control macros retain their existing name-to-parenthesis gap. In parsed `for`
headers, `sp_after_semi_for` applies after a semicolon before a nonempty clause,
`sp_between_semi_for_empty` controls adjacent semicolons and
`sp_after_semi_for_empty` controls an empty final clause. For example, `add` for
the first option changes `for(uint i = 0;;++i)` to
`for (uint i = 0;; ++i)` without changing the adjacent empty-clause semicolons.

# Call interiors and conditional labels

`sp_inside_fparen = add` inserts missing spaces inside nonempty function calls,
constructor initializer calls and macro calls. Existing
wider gaps remain. With `sp_paren_paren = remove`, consecutive closing
parentheses use the actual emitted gap before the inner `)`: if it has a space,
the outer `)` stays adjacent; otherwise the enclosing call's spacing is used
after the inner `)`. Thus both `static_assert( sizeof(void*) )` and
`static_assert( sizeof( void* ));` retain their closing gaps. This also covers
empty lists, casts and grouped expressions: `Call( Empty() )`,
`switch ( uint(mode) )` and `Call( (value) )`. An enclosing cast keeps its own
compact interior: `float(GetValue())` and `float(uint(value))`. This decision
uses output spacing, including changes made earlier on the same line, so
repeated runs are stable. Empty lists use `sp_inside_fparens`.
Declaration interiors continue to use `sp_inside_fparen_decl` independently.
Ordinary grouping parentheses do not use the enclosing-call fallback:
`created = (dst[i].IsValid());` stays compact. `sp_paren_paren` and
`sp_nested_close_brace_paren` can join grouping closers without inserting a
call-interior space. `alignas` and requires-expression parameters follow their
declaration-interior rules instead.
Explicit call boundaries apply on retained rows and next to pointer/reference
arguments. Comments, line breaks, directives and Objective-C message rows remain
protected. Opaque code-bearing macros receive outer argument-list spacing;
their bodies remain opaque. Braced initializers keep their existing gaps,
except at the closing boundaries explicitly controlled below.
An opening braced argument touches its call parenthesis: `Call({ value })` and
`DEBUG_ONLY({ ... })`. An active call-interior rule removes an existing gap at
this boundary; `ignore` retains it. Comments and line breaks still take priority.
When the emitted-gap rule below adds or retains a space between a compact final
call and an initializer's closing brace, its matching opening brace receives
a minimum inner space too. Thus `Freq{uint(value)}` and `Box{RVRef(value) }`
become `Freq{ uint(value) }` and `Box{ RVRef(value) }`. Only single-line parsed
initializers with this `)}` boundary participate; compact scalar initializers,
empty braces, function bodies, comments and multiline layouts keep their gaps.
The opening gap is planned before argument-name and suffix alignment.
Fully compact initializers do not receive enclosing spaces. Built-in and
configured `TYPE` brace initializers use `sp_inside_paren_cast`, so
`usize{ key.CalcHash() }` becomes `usize{key.CalcHash()}` with `remove`.
Spaced non-scalar initializers still mirror their opening space as above.

Names configured with `set FUNC_CALL_USER` use
`sp_func_call_user_inside_fparen` for their nonempty call interiors. For example,
`set FUNC_CALL_USER RVRef` with `remove` produces `RVRef(*this)` and
`Call( RVRef(value) )`. A compact special call can contain an ordinary spaced
call: `RVRef(Call( value ))`. The parser still treats declarations and definitions
of the configured name as declarations and definitions; their parameter lists
keep declaration spacing and namespace indentation. Empty calls remain governed
by `sp_inside_fparens`. Comments, newlines and protected macro contents remain
unchanged.

`sp_nested_close_brace_paren = remove` applies the same emitted-gap rule between
`}` and `)` or between `)` and `}`. With `sp_inside_fparen = add`, a compact
inner closer receives a space after it, while an inner closer with a preceding
space stays adjacent to the outer closer. Thus `if ( mode == Mode{"PrivateEngine"} )`
retains its space, and `Call( Thing{ Value() } )` becomes
`Call( Thing{ Value() });`. Declaration and cast interiors still use their own
options. The option affects only preservation mode and defaults to `ignore`.
Comments, line breaks, directives and opaque macro bodies remain protected.

Namespace function indentation uses the function signature as its anchor,
including for constructors with initializer lists. Initializers, body braces,
statements and comments shift uniformly with the signature; a statement-start
flag on a member initializer does not establish a separate indentation anchor.
Already indented definitions retain their relative layout, and inline class
members are excluded from the namespace rule.

`sp_inside_paren_cast` controls recognized casts separately from calls. With
`remove`, `float( value )` becomes `float(value)` while `Call( 42 )` remains
spaced. C-style casts and named C++ casts use this option too. A function-style
cast must name an original `TYPE` token, including built-in types and scalar
aliases explicitly configured with `set TYPE`. The original classification is
required because the parser can label an ordinary call in a default argument
as `CPP_CAST`. Unknown names continue to use call spacing. Nested calls keep
their own spacing: `float(Call( 42 ))` and `Call( float(value) )`. Comments,
line breaks and Objective-C message rows retain their layout.
Spaced function-type template arguments such as `Function<bool (const Path &)>`
retain their interiors and parameter alignment; they can also receive the
parser's `CPP_CAST` label. Direct `void` function-type arguments are protected
even when the type touches `(`.

`mod_add_long_ifdef_endif_comment` enables labels on uncommented `#endif`
directives when the opening-to-closing line distance exceeds the configured
threshold. Zero disables the rule. The entire conditional, including nested
blocks and `#elif`/`#else` branches, is counted. Declaration splits planned in the
same pass count too, as do inserted namespace brace newlines, avoiding a
second-pass change at the threshold.
The label is the opening condition (`#ifdef FOO` becomes `// FOO`,
`#ifndef FOO` becomes `// ifndef FOO`). Continued conditions are flattened onto
one comment line and opening comments are omitted. A single positive
`defined(NAME)` or `defined NAME` is shortened to `NAME`. An existing line
comment containing exactly that matching defined expression is shortened too;
compound conditions, negations, unrelated labels, extra comment text and block
comments are retained. These are the only new comments
emitted by preservation mode; all original directive text is retained.

Control interiors use `sp_inside_sparen` and its opening/closing overrides;
`for` uses the separate `sp_inside_for` options. They apply even on retained
rows without changing declaration or cast interiors. `sp_after_comma` edits
the horizontal gap following a parsed comma, preserving comments, newlines,
protected text, and trailing commas before closing brackets.

# Statement newlines and compact member width

Preservation mode supports `nl_fdef_brace = add`, `nl_after_vbrace_open = true`
and `nl_after_semicolon = true`. Function opening braces move to a new line,
unbraced control bodies receive a newline and one additional indentation level,
and statements following semicolons receive separate lines. Expanded braced
bodies also separate their first statement and closing brace. Existing newlines
remain unchanged. Loop-header semicolons, requirement bodies, call arguments,
protected macro contents, comments and Objective-C message rows are excluded.
Constructor initializer lists use the function signature as their indentation
anchor. Inserted newlines count toward conditional-comment thresholds and
member-group alignment spans in the same pass.

`nl_func_leave_one_liners = true` preserves compact direct class/struct member
definitions. `nl_func_one_liner_max_columns` limits their visual width after
spacing and alignment, including indentation, expanded tabs and comments.
The default is zero (unlimited). A nonzero limit expands longer member bodies
using the configured newline rules without wrapping long expressions or
signatures. `nl_cpp_lambda_leave_one_liners` independently preserves compact
lambdas. The width limit only affects preservation mode.

Rows containing a parsed `case` or `default` label retain their compact statement
layout, including `case VALUE : Work(); break;`. They are excluded from the
statement-newline planner. Existing multiline cases remain multiline.

`sp_before_for_colon` and `sp_after_for_colon` control range-for separators,
independently of case, bit-field, inheritance and conditional-expression colons.
With both set to `add`, `for (uint x:a)` becomes `for (uint x : a)`.

Namespace indentation also covers direct declarations, using directives and
aliases, template headers, and `//` comments. Unindented namespace-level types
shift together with their bodies, preserving relative member indentation.
Multiline template and using headers shift as a unit. Block comments beginning
with `/*` retain their original columns, including within shifted definitions.
Preprocessor rows, protected macros, and anonymous namespaces keep their layout.

`nl_template_func`, `nl_template_class` and `nl_template_using` support `add`
before the declaration following `template<...>`. Function declaration/definition
overrides are also honored. Existing constraints after a template header retain
their layout. Planned template newlines count toward suffix alignment spans and
conditional-comment thresholds in the same pass.

`nl_enum_own_lines = add` places each enumerator on a separate line, with the
first value after `{` and the closing brace after the last value on their own
lines. Only commas at the enum body's level separate entries; commas inside
calls, templates and initializer lists retain their context. Empty enums remain
compact. The AE configuration disables `nl_enum_leave_one_liners`.

When unbraced control bodies move onto new lines, existing braced body rows and
closing braces follow the emitted control header. Thus a chained `for (...) for
(...) { ... }` receives matching indentation for the inner loop, its body and
its closing brace. This also repairs a previously outdented closing brace.
Body rows keep their relative layout when the header moves. Repairing an
existing outdented brace keeps adequately indented rows in place and indents
rows below the minimum body level. Namespace shifts are applied through the
header anchor, so repeated formatting is stable.

`align_var_class_span` and `align_var_struct_span` enable alignment of consecutive
field declarations in classes and structs/unions, respectively. Their `gap`
options set the minimum separation after the type. Only the first name on a
declaration participates, preserving additional declarators on that row and
their initializers. Nested templates, arrays and attached pointers/references
are supported. Groups end at blank lines (with span 1), access labels, methods,
preprocessor directives and other intervening declarations. Compact single-line
types retain their layout. Existing aligned groups keep their columns, and
`align_with_tabs` selects tab stops for newly aligned groups. Local-variable
alignment continues to use `align_var_def_span` and `align_var_def_gap`.

`mod_split_field_decl` optionally replaces comma-separated class/struct/union
field lists with separate declarations, repeating the shared declaration
specifiers. Per-field pointer/reference qualifiers, array bounds, bit widths
and initializers retain their meaning and order. Type-definition bodies,
typedef/using declarations, function/member pointers, protected macro text,
comment-containing lists and compact single-line types are excluded. The rule
is disabled by default and only applies in preservation mode.

Splitting runs after symbol classification and before layout planning. When a
list changes, the native pipeline reparses the rewritten source once. Alignment,
namespace shifts, conditional-comment thresholds, encoding and newline handling
then use the split declarations directly. No external formatter script is used.

`align_field_assign_span` aligns the `=` tokens of consecutive direct field
initializers. A value of zero disables it. Blank lines (with span 1), access
labels, methods and other intervening declarations separate groups. Comma lists
that were not split are excluded from assignment alignment. Field names are
measured first, then assignments align using the actual emitted columns;
existing aligned columns are retained. Local assignments and pure/defaulted
methods remain outside this rule.
