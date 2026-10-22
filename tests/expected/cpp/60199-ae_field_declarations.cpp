struct Invocation
{
	Vec<uint32_t, 3>	global;
	Vec<uint32_t, 3>	local;
	Vec<uint32_t, 3>	group;
	Vec<uint32_t, 3>	groupCount;
	Vec<uint32_t, 3>	groupSize;
	uint32_t			localIndex = 0;
};

struct BufferPtr
{
	const BufferDesc*	buffer;
	size_t				offset			= 0;
	size_t				componentStride = 4;
	bool				readable		= true;
	bool				writable		= true;
};

class Declarators
{
	const int	   *pointer;
	const int		value		= 1;
	const int	   *other		= nullptr;
	int				array[2]	= {1, 2};
	int				second[2]	= {3, 4};
	int			   &reference	= source;
	int				plain		= 2;
	Pair<int, int>	first		= Make( 1, 2 );
	Pair<int, int>	last{3, 4};
	unsigned		bits : 2;
	unsigned		moreBits : 3;

	int separate = 0;
public:
	int afterAccess = 1;
	int longerName	= 2;
	void  Method ();
	int afterMethod = 3;
	int next		= 4;
};

struct Preserved
{
	int a, /* keep this comment */ b;
	int c, // keep line comment
		d;
	void (*first)(int), (*second)( int );
	int Owner::*one, Owner::*two;
	struct { int member; } firstObject, secondObject;
	TYPE_MACRO({ int x,y; });
};
struct Compact { int a, b; };

struct Nested
{
	struct Child
	{
		int		x;
		int		y;
		bool	a		= true;
		bool	longer	= false;
	};
	Child	child;
	int		z;
	int		w;
};

void  Locals ()
{
	int a = 0, b = 1;
	int c = 0;
	int longer = 1;
	for (int x = 0, y = 1; x < y; ++x) {}
}

namespace Shifted
{
	struct Indented
	{
		int		x;
		int		y;
		bool	a			= true;
		bool	longName	= false;
	};
}
#if defined(TEST_FIELDS)
struct Conditional
{
	int a;
	int b;
	int c;
	int d;
};
#endif // TEST_FIELDS
