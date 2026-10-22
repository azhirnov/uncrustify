void Calls(int x)
{
    CHECK_ERR(FileSystem::IsDirectory(_tempFolder));
    Call(Other(value) );
    Call((value));
    static_assert( sizeof(void*) );
    static_assert( sizeof(void*));
    static_assert( sizeof( void* ));
    Call( sizeof(void*) );
    Call( sizeof( void* ));
    Call( (value));
    Call( Other( sizeof(void*)) );
    Call( (value)	);
    Call(&x); Call(*ptr); Call("a  b");
    Call( /* keep */ value /* keep */ );
    Call(
        value
    );
    Empty(); Empty( );
    FOO([] { return 1; }());
    FOO({ int x=0; });
    rt->_usage.push_back({ order, access });
    rt->_usage.push_back( { order, access });
    GFX_DBG_ONLY({
        int value=0;
        Call(value);
    });
    GFX_DBG_ONLY( {
        Call(value);
    });
    Call( /* keep brace comment */ { 1, 2 });
    Call(
        { 1, 2 });
    uint width = simd?_SimdWidth():0;
    auto nestedConditional = a?b?c:d:e;
    auto addresses = a?*ptr:&value;
    auto retainedConditional = a	?	b	:	c;
    auto commentedConditional = a? /* keep */ b:c;
    auto splitConditional = a?
        b:
        c;
    RVRef(*this); RVRef( *this );
    RVRef( Call( value ));
    RVRef( RVRef( value ));
    Call(RVRef(*this));
    RVRef( uint( value ));
    RVRef( /* keep */ value /* keep */ );
    RVRef(
        value
    );
    DWORD( value ); UINT( Call(value) ); SIZE_T( value );
    DWORD_PTR( value ); ULONGLONG( value );
    auto vectorByte = ubyte2( value );
    auto vectorInt = int4( 1, 2, 3, 4 );
    auto vectorFloat = float3( value );
    auto vectorPacked = packed_float3( value );
    auto braceCompactCall = Box{RVRef(value) };
    auto braceCompactCast = Freq{uint(freq + 0.5f) };
    auto braceNewGap = Box{RVRef(value)};
    auto braceNested = Box{uint(Call(value))};
    auto bracePlain = Box{42};
    for(uint i = 0;;++i) {
        Path candidate = _tempFolder/("temp-"s << ToString( i ));
        CHECK_ERR( i<10000 );
    }
    for(int i=0;i<4;++i) { Call(i); }
    for(;;) { break; }
    auto arithmetic = a+b*c/d%e-f;
    auto shift = a<<2;
    auto bits = (a&b)|c^d;
    auto unary = -value;
    int* pointer = &value;
    auto pointed = *pointer;
    Call(OUT &value, INOUT *pointer);
    result = BrotliDecoderDecompressStream( static_cast<BrotliDecoderState *>(instance), INOUT &available_in, INOUT &next_in,
                                           INOUT &available_out, INOUT &next_out, null );
    while(value<=4) { ++value; }
    ptr->~T(); auto bits = ~ToNearUInt(x);
    auto result = object.operator bool();
    auto v = Vector<T>(x);
    auto duration = float( _duration );
    auto count = uint( value );
    auto size = usize( value );
    auto nestedCast = float( Call( 42 ));
    auto nestedCall = Call( float( value ));
    Uniform<usize>( 0u, names.size());
    Uniform<usize>( 0u, names.size() );
    switch ( uint(mode)) { default: break; }
    switch ( uint(mode) ) { default: break; }
    if ( Empty() )  return;
    if ( Empty())  return;
    if_unlikely( not Check( value, False{"weak"} )) return;
    Call( static_cast<float>(value));
    Call( Call());
    Call( Call() );
    auto castEmpty = float( GetValue() );
    auto castCast = float( uint(value) );
    if ( mode == Mode{"PrivateEngine"} ) {}
    if ( mode == Mode{"PrivateEngine"}) {}
    if ( mode == Mode{ "PrivateEngine" } ) {}
    Call( Thing{"value"});
    Call( Thing{ "value" } );
    Call( Thing{});
    Call( Thing{ Other(value) } );
    auto castBraced = float( Thing{} );
    symbol_supplier.reset( new SimpleSymbolSupplier{ symbolsPath.string() } );
    symbol_supplier.reset( new SimpleSymbolSupplier{ symbolsPath.string() });
    Call( Thing{ Other() } );
    Call( Thing{ Other() /* keep */ } /* keep */ );
    Call( Thing{ Other()
    }
    );
    FOO([] { return 1; }(), Call());
    FOO({ return uint(mode); }, uint(mode));
    FOO({ return 1; }, static_cast<uint>(mode));
    FOO({ return 1; }, static_cast<Alias<T>>(mode));
    auto namedCast = static_cast<float>( value );
    auto cCast = ( float ) value;
    auto pointerCast = ( int* ) ptr;
    auto castComment = float( /* keep */ value /* keep */ );
    auto castNewline = float(
        value
    );
}
class C {
    C(int x) : Base(x), member(Call(x)) {}
    C(char) : C(Call(42)) {}
    C(long x) : member{x} {}
    C(Stream stream) : _dataStream{RVRef(stream) } {}
    void F(int x = Call(42));
    void BracedDefault(Thing value = Thing{});
    template <typename ...Args>
    void  Emplace (Args&& ...args) __NE___;
};
#define CALL(X) F(X)
namespace MoveFunction
{
template <typename T>
T&& RVRef (T& value) __NE___
{
    return static_cast<T&&>(value);
}
}
void  TemplateSignature (Function<void ( Args... )> callback);
void  TemplateSignature2 (Function<bool ( const Path & )> callback);
void  TemplateSignature3 (Function<float ( T )> callback);
template <typename To, typename From>
concept ExplicitlyConvertibe = requires (From from) { To{from}; };
template <typename To, typename From>
concept ExplicitlyMoveConvertibe = requires ( From from ) { To{RVRef(from)}; };
template <typename To, typename From>
concept Copyable = requires ( To to, From from ) { to = from; };
template <typename T>
concept Combined = Pred<T> && requires ( T value ) { Call(value); };
template <typename ...Args>
concept Invocable = requires( Args&& ...args ) { Call(args...); };
template <typename T>
concept EmptyRequires = requires( ) { T{}; };
template <typename T>
concept CommentRequires = requires ( /* keep */ T value /* keep */ ) { Call(value); };
template <typename T>
concept NewlineRequires = requires (
    T value
) { Call(value); };
template <typename T> requires requires ( T value ) { Call(value); }
void NestedRequires();
template <typename T> void Constraint() requires ( Pred<T> ) {}
template <typename T> void TightConstraint() requires(Pred<T>) {}
template <typename T> void CallConstraint() requires(Check<T>()) {}
template <typename T> requires( Pred<T> ) void LeadingConstraint();
const char* raw = R"(CHECK_ERR(Foo(x));
#if FAKE
#endif
)";

struct alignas(AE_CACHE_LINE) PerFrame {};
struct alignas( AE_CACHE_LINE ) Aligned {};
struct alignas(alignof(int)) NestedAligned {};
void Contexts()
{
    created = (dst[i].IsValid());
    repaired = (dst[i].IsValid() );
    auto grouped = (uint(value));
    auto braced = (Thing{Call(value)});
    if (simd) { Call(first,second); }
    if constexpr (bool(value)) {}
    for(uint i=0;;++i) {}
}
namespace AE::GlslToSimd {
bool Compiler::_CompileSPIRVtoDLL (const String& cppSource,OUT OutputDLL& output,bool simd) { return true; }
}
namespace { void Anonymous() {} }
namespace Outer
{
    namespace Inner {
    void Nested() {}
    }
}
#if OUTER
#ifdef INNER
int first;
int second;
int third;
int fourth;
int fifth;
int sixth;
#endif
#elif OTHER
int seventh;
#else
int eighth;
#endif
#ifndef GUARD
int a;
int b;
int c;
int d;
int e;
int f;
#endif
#if SHORT
int short_value;
#endif
#if EXISTING
int a;
int b;
int c;
int d;
int e;
int f;
#endif /* user comment */
#if defined(A) && \
    defined(B) /* opening comment */
int a;
int b;
int c;
int d;
int e;
int f;
#endif
#if A/**/&&/**/B
int a;
int b;
int c;
int d;
int e;
int f;
#endif
#if 0
#if DISABLED
int a;
int b;
int c;
int d;
int e;
int f;
#endif
#endif
#if defined(AE_ENABLE_VULKAN)
int a;
int a;
int a;
int a;
int a;
int a;
#endif
#if defined AE_ENABLE_METAL
int a;
int a;
int a;
int a;
int a;
int a;
#endif
#if defined(AE_ENABLE_VULKAN)
int a;
int a;
int a;
int a;
int a;
int a;
#endif // defined(AE_ENABLE_VULKAN)
#if defined ( AE_ENABLE_METAL )
int a;
int a;
int a;
int a;
int a;
int a;
#endif // defined AE_ENABLE_METAL
#if defined(KEEP)
int a;
int a;
int a;
int a;
int a;
int a;
#endif // defined(OTHER)
#if defined(KEEP)
int a;
int a;
int a;
int a;
int a;
int a;
#endif // defined(KEEP) custom label
#if !defined(NEGATIVE)
int a;
int a;
int a;
int a;
int a;
int a;
#endif
#if defined(A) || defined(B)
int a;
int a;
int a;
int a;
int a;
int a;
#endif
