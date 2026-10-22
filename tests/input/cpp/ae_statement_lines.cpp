namespace AE::GlslToSimd
{
	bool  Compiler::_CompileSPIRVtoDLL (const String&, OUT OutputDLL& output, bool simd) {
		if (simd) cmake += value;
		output.lib.Unload(); return false;
	}
}
void F()
{
	if (first) if (second) Call(); else Other();
	if (first) { Call(); return; }
	for (int i=0;i<3;++i) Call();
	for (;;) { break; }
	auto lambda = [] { Foo(); return 1; };
	KEEP({ Foo(); return; });
	Foo(); /* retain comment */ Bar();
	static_assert( requires{ typename T::type; });
	StaticAssert( IsConstExpr( Unused(CxFunction(1)); ));
	return usize{key.CalcHash()};
}
struct S
{
	int Short () { int b=1; Foo(); return b; }
	int Long () { Call(argument0,argument1,argument2,argument3,argument4,argument5,argument6,argument7,argument8,argument9,argument10,argument11,argument12,argument13,argument14,argument15,argument16,argument17); int b=1; long c=2; return b; }
	template <typename T, typename T2, typename R>
	bool Cast (T left, T2 right) { return T( right ) == left and T2( left ) == right and R( left ) == R( right ); }
	usize Hash () { return usize{ key.CalcHash() }; }
	int Empty () {}
};
void CastBraces()
{
	return usize{key.CalcHash()};
	auto tight = Box{RVRef(value)};
	auto spaced = Freq{uint(freq + 0.5f) };
	auto comment = usize{ /* keep */ key.CalcHash() /* keep */ };
}
namespace Constructors
{
	JavaEnv::JavaEnv (JNIEnv* env) __NE___ :
		_env{ env } { Attach(); Work(); }
}
class ExpandedGroups
{
	Strong<ComputePipelineID>  One () __NE_OV { Call("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"); }
	Strong<RayTracingPipelineID>Two () __NE_OV { Call("yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy"); }
};
template <typename T, int I, glm::qualifier Q> requires(IsFloatPoint<T>)
ND_ TVec<T,I,Q>&  operator %= (TVec<T,I,Q> &lhs, const TVec<T,I,Q> &rhs) __NE___
{
	return (lhs = glm::mod( lhs, rhs ));
}
void OperatorCalls()
{
	return operator+(left,right);
	value.operator+=(other);
}
namespace Layout
{
using namespace Other;
template <typename First,
          typename Second>
using Pair = std::pair<First, Second>;
// namespace comment
/* banner
 * retain this column
 */
template <typename T, size_t N> struct Vec
{
	T value;
/* class banner */
	// class comment
};
struct BufferPtr
{
	int value;
};
template <typename T, size_t N> void WriteVec (BufferPtr p, const Vec<T,N>& value)
{
	for (uint x:a) { Use(x); }
	for (uint x : a) { Use(x); }
	switch (result)
	{
		case FAILURE : msg << "FAILURE"; break;
		default : msg << "OTHER"; return;
	}
	Work(); return;
}
template <typename T, size_t C, size_t R> using Matrix = std::array<Vec<T,R>,C>;
}
enum class Op { Add, Subtract, Multiply, Divide, Remainder, Modulo, Min, Max,
	And, Or, Xor, ShiftLeft, ShiftRight, Equal, NotEqual, Less, LessEqual, Greater, GreaterEqual };
enum Values { First = Call(1,2), Second = Pair<int,int>{1,2}, Third, };
enum Empty {};
enum Comments { One, /* retain block comment */ Two, // retain line comment
	Three };
void NestedLoops()
{
	for (int y=0; y<2; ++y) for (int x=0; x<2; ++x) {
		Use(x,y);
	}
	for (int z=0; z<2; ++z) for (int y=0; y<2; ++y) for (int x=0; x<2; ++x) {
		Use(x,y,z);
	}
	for (int y=0; y<2; ++y)
		for (int x=0; x<2; ++x) {
		Use(x,y);
	}
	for (int y=0; y<2; ++y) for (int x=0; x<2; ++x)
	{
		Use(x,y);
	}
}
struct LocalEnums
{
	int One () { enum E { A, B }; return A; }
};
void MisplacedClose()
{
	if (enabled) {
		First();
						Second();
					}
	for (int y=0; y<2; ++y)
		for (int x=0; x<2; ++x) {
			Use(x,y);
	}
}
void LocalClassInLoops()
{
	for (;;) for (;;) {
		struct S
		{
		public:
			int value;
		};
	}
}
void ExistingOutdentedBraces()
{
	for (;;)
		for (;;)
	{
		Work();
	}
}

struct PacketInvocation {
	Vec<Packet<uint32_t>, 3> global, local;
	Vec<uint32_t, 3> group, groupCount, groupSize;
	Packet<uint32_t> localIndex;
	uint32_t subgroupID = 0, numSubgroups = 0;
};

class FieldGroups
{
	int first;
	LongType second;

	int separate;
public:
	int afterAccess;
	LongerType other;
	void Method ();
	int afterMethod;
	OtherType next;
private:
	int *pointer;
	OtherType &reference;
	int array[3];
	static constexpr uint flags = 0;
};

struct FieldInitializers
{
	int count = Call(1,2);
	OtherType value{1,2};
	uint tail;

	struct Nested
	{
		int inner;
		OtherType nested;
	};
	int afterNested;
	OtherType last;
};

struct CompactFields { int one; LongType two; };
union UnionFields
{
	int integer;
	OtherType value;
};
