static bool CreatePipeline (TestDevice &device, Shader shader,
						   OUT Pipeline &pipeline);

static bool  RepairPipeline (TestDevice &device, Shader shader,
							OUT Pipeline &pipeline);

float Free1 ();
int Free2 ();

template <typename A, typename B> ND_ auto  Minimum (const A x, const B y) -> decltype(A(1) + B(1)) requires(std::is_scalar_v<A> and std::is_scalar_v<B>);

void  Indirect (MetalBuffer	indirectBuffer,
				Bytes		indirectBufferOffset,
				uint		drawCount,
				Bytes		stride);

void  Mixed (int first, long second,
		uint third, short fourth);

void  Nested (Pair<int, int> first,
		uint second = Make(1, 2),
		const Thing &third,
		float values[4]);

void  FreeShort () __NE_OV;
void  FreeLongName ()		__NE_OV;

class SuffixColumns
{
	void  GetLastState (OUT CaptureResponse &)			C_NE_OV;

	// IInputSurface //
	void  SetSamplerCache (ArrayView<SamplerName>)		__NE_OV;
	AsyncTask	Begin (CommandBatchPtr beginCmdBatch,
					   CommandBatchPtr endCmdBatch,
					   ArrayView<AsyncTask> deps)		__NE_OV;
	AsyncTask	End (ArrayView<AsyncTask> deps)			__NE_OV;
	bool		GetImages (OUT SampledImages_t &)		C_NE_OV;

private:
	void  Separate () __NE_OV;




	void  FarAway ()		__NE_OV;
};

class BeyondLimit
{
	void  Short ()																									__NE_OV;
	void  Longer ()																									__NE_OV;
};

class OverflowByArguments
{
	void  First (Array<VeryLongElementTypeWithEnoughLettersToWidenArgumentNames> first,
				 int argumentNameWithEnoughLettersToCrossTheLimit) __NE_OV;
	void  Second () __NE_OV;
};

class Commands
{
public:
	ND_ VkCommandBuffer	EndCommandBuffer () __Th___;
	ND_ VCommandBuffer  ReleaseCommandBuffer () __Th___;

	float  Short ();
	int  Long ();

	float	Tab1 ();
	LongType	Tab2 ();

	float	 Wrapped (int first,
					int second);
	LongType	 Other (int first,
					int second);

	ND_ bool	Equal (const Self &rhs, const T err = Epsilon<T>())		C_NE___	{ uint b = 1;  for (auto i : Indices) b &= uint(Base::Equal( _values[i], rhs._values[i], err ));  return b == 1; }
};

class ConversionSuffix
{
	ND_ uint		Columns ()						C_NE___ { return _dim.columns; }
	ND_ MatrixDim	Dimension ()						C_NE___ { return _dim; }

	ND_ explicit operator ArrayView<RemoveConst<T>> ()	C_NE___ { return { _ptr.get(), _dim.Size() }; }
};

class ConditionalSuffix
{
	ND_ Self	Abs ()									  C_NE___ { return value; }
	ND_ Self	PreciseSqrt ()							  C_NE___ { return value; }

	ND_ Self	Reciprocal_fp32 ()						  C_NE___;
  #if AE_SIMD_AVX >= 31
	ND_ Self	Reciprocal ()							C_NE___ { return value; }
  #elif AE_SIMD_AVX >= 20
	ND_ Self	Reciprocal ()							C_NE___ { return value; }
  #else
	ND_ Self	Reciprocal ()							C_NE___ { return Reciprocal_fp32(); }
  #endif
	ND_ Self	FastDiv (const Self &rhs)				C_NE___ { return value; }
};

class OptionalSuffix
{
	__Cx__ OptionalSuffix (Iterator b, Iterator e)__NE___ : _begin{b}, _end{e} {}
};

class TouchingSuffix
{
	void  A ()__NE___;
	void  B ()__NE___;
};

class CapacitySuffix
{
	NdCx__ static usize	capacity ()						__NE___ { return ArraySize; }
	ND_ explicit operator ArrayView<T> ()				__NE___ { return view; }
};

class AlignedSuffixSpaces
{
	void  First ()        __NE___;
	void  Second ()       __NE___;
};

class DirectiveBoundary
{
	void  Before ()__NE___;
#define SOME_MACRO 1
	void  After ()		__NE___;
};

class OptionalWithPeer
{
	void  Short ()__NE___;
	void  LongerName ()		__NE___;
};

class LimitAfterRounding
{
	void  First ()                                                                                 __NE___;
	void  Second ()                                                                             __NE___;
};

class ExplicitOperatorCall
{
	bool value = other.operator bool();
	void  Before ()__NE___;
	bool another = other.operator bool();
	void  After ()		__NE___;
};

class DecoratedParameters
{
	static bool  FindUnusedFilename (const Function< void (OUT Path &, usize idx) >	&buildName,
									 const Function< bool (const Path &) >			&consume,
									 uint											maxStep = 1000)						__Th___;
};

void  AlignPointers (LongType *first,
					Short *second,
					int count);

class PlainConst
{
ND_ EPixelFormat				GetIntermediateFormat ()	const { return _intermPixFmt; }
ND_ Graphics::VDevice const&	GetDevice()					const { return _vulkan; }
};

class PlainConstSpacing
{
	int  First () const;
	int  SecondName () const;
};

template <typename T>
class PackSuffixes
{
	__Cx__ bool  try_push_back (const T &value)			__NE___;
	__Cx__ bool  try_push_back (T&& value)				__NE___;

	template <typename ...Args>
	__Cx__ bool  try_emplace_back (Args&& ...args)		__NE___;

	__Cx__ void  insert (usize pos, const T &value)				__NE___ { insert( pos, T{value} ); }
	__Cx__ void  insert (usize pos, T &&value)					__NE___;

	__Cx__ void  resize (usize newSize)							__NE___;
	__Cx__ void  resize (usize newSize, const T &defaultValue)	__NE___;
};

template <typename T>
class PackAfterConstructor
{
	explicit PackAfterConstructor (const Allocator_t &alloc = Allocator_t{})	__NE___;
	~PackAfterConstructor ()													__NE___;

	ND_ BaseList_t	Release ()											__NE___;

	template <typename ...Args>
	ND_ bool  Emplace (Args&& ...args)									__NE___;

		void  Destroy ()													__NE___;

	NdCx__ static Bytes  ChunkSize ()										__NE___ { return Bytes{_ChunkSize}; }
};

inline std::error_code make_error_code( Result e ) noexcept
{
    return std::error_code( e );
}

void  DeclarationInteriors ( int value = call( 42 ) );
void  UnnamedPointer ( int * );
void  NestedDefault ( Type value = Type( 42 ) );
void  CommentBoundary ( /* keep */ int value /* keep */ );
void  NewlineBoundary (
    int value
);

class RetainedRowInteriors
{
    void  Member (	int value	) __NE___;
};

class SystemError : public Error, public std::system_error
{
public:
SystemError();
SystemError( std::error_code ec ) : Error(), std::system_error( ec ) {}
SystemError(int ec) : SystemError( std::error_code( ec ) ) {}
SystemError(char) : SystemError() {}
SystemError(const SystemError &) = default;
~SystemError() {}
};

SystemError::SystemError() : Error(), std::system_error() {}

void  ConstructorLikeCalls (SystemError *ptr)
{
ptr->~SystemError();
value.~T();
auto bits = ~ToNearUInt( 42 );
auto mask = ~BitMask();
auto value = ~uint(42);
}

void  AlignedUnnamed (AVColorTransferCharacteristic	colorTrc,
					  AVColorSpace					colorSpace,
					  AVChromaLocation				) __NE___ {}

void  RepairUnnamed (AVColorTransferCharacteristic	colorTrc,
					 AVColorSpace					colorSpace,
					 AVChromaLocation) __NE___ {}

void  UnnamedReference (AVColorTransferCharacteristic	colorTrc,
						AVColorSpace					colorSpace,
						const AVChromaLocation &		) __NE___ {}

void  CompactUnnamed (AVChromaLocation     ) {}
void  MixedUnnamed (int first, int second,
                   AVChromaLocation		) {}
void  NamedFinal (AVColorSpace first,
                 AVChromaLocation second   );
void  UnnamedOnSeparateLine (AVColorSpace first,
                            AVChromaLocation
) {}

namespace Outer
{
namespace
{
	static bool NestedFunction( int value ) { return Call( value ); }
	int  WideFunction( int value ) { return Call( value ); }
	int* PointerFunction( int value ) { return nullptr; }
	int &ReferenceFunction( int value );
	int	TabbedFunction( int value );
	int /* keep */ CommentFunction( int value );
	int OperatorFunction( int value ) { return object.operator int(); }
	void  Calls () { Call( 1 ); ptr->~SystemError(); auto bits = ~BitMask(); }
}
	uint Compiler::Width() const { return 4; }
	uint Namespace::Compiler::WidthWithNamespace() const { return 4; }
	uint /* keep */ Compiler::CommentWidth() const { return 4; }
	int* Compiler::PointerWidth() const { return nullptr; }
	int& Compiler::ReferenceWidth() const;
	Compiler::Compiler(int value) : Compiler( Call( value ) ) {}
}

class TightTemplateReturn
{
	Strong<LongType>First();
	Strong<T>		Second();
};

namespace TemplateQualification
{
	template <typename T>
	int Holder<T>::Value() const { return 1; }
	template <typename T>
	int  Holder<T>::AlreadySpaced () const { return 1; }
	template <typename T>
	int NS::Holder<T>::Qualified() const { return 1; }
	template <typename T>
	int Holder<Vector<T>>::Nested() const { return 1; }
	template <typename T> template <typename U>
	int Holder<T>::Inner<U>::Deep() const { return 1; }
}

namespace NativeLayout
{
namespace {
	bool  ExecAndPrint (String command, const Path& path)
	{
		String log; bool success=false;
		bool completed=RunProcess(command,path,OUT log,OUT success);
		output.fn=null;
		output.mask|=bits;
		int *pointer=nullptr;
		long count=1;
		for (int i=0; i<3; ++i) { int x=1; int y=2; Use(x,y); }
		KEEP( int value=1; );
		return completed and success;
	}
}
bool  Compiler::Build (const String &source)
{
	String log; bool success=false;
	bool completed=RunProcess(source,OUT log,OUT success);
	const char* raw=R"(literal=1;
zero-indent raw content
)";
	// retain this comment's relative indentation
	output.fn=null;
#ifdef MODE
	output.fn=ptr;
#else
	output.fn=null;
#endif
	return completed and success;
}
namespace Deep {
void  NestedNamed ()
{
	first=second;
}
}
}
void  InlineDeclarations () { int first=1; int second=2; Use(first,second); }

class InlineTemplateAssignment
{
	template <uint I=0>	ND_ auto	ToShort ()	C_NE___;
	template <uint I=0>	ND_ auto	ToLong ()	C_NE___;
};

namespace DefaultInitializer
{
bool  WithLambda (Fn callback = [] () {
	return true;
})
{
	bool value=false;
	return callback() and value;
}
}

class MixedTemplateAssignment
{
	template <uint I=0>	ND_ auto	ToFloat ()	C_NE___;
						ND_ auto	ToHalf ()	C_NE___;
						ND_ auto	ToByte ()	C_NE___;
	template <uint I=0>	ND_ auto	ToInt ()		C_NE___;
};

namespace ConstructorIndent
{
	JavaEnv::JavaEnv (JNIEnv* env) __NE___ :
		_env{ env }
	{
		Attach();
	}

	JavaEnv::JavaEnv (int value) __NE___ :
		_first{ value },
		_second( Call( value ) )
	{
		// retain constructor body indentation
		if ( value )
		{
			Attach();
		}
	}

	JavaEnv::JavaEnv (char value) :
		JavaEnv( int(value) )
	{
#ifdef MODE
		Attach();
#endif
	}

	JavaEnv::JavaEnv (Fn callback) :
		_callback{ [] () {
			Call( 42 );
		} }
	{
		Attach();
	}

namespace Deep
{
JavaEnv::JavaEnv (short value) __NE___ :
	_env{ value }
{
	Attach();
}
}

		JavaEnv::JavaEnv (long value) :
			_env{ value }
		{
			Attach();
		}

	class InlineConstructor
	{
		InlineConstructor (int value) :
			_env{ value }
		{
			Attach();
		}
	};
}
