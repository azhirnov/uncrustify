#define TABLE( _visit_ ) \
	_visit_( First )		\
	_visit_( Second )	// keep macro tabs

float  Func1 ();
int  Func2 ();

void  Empty ();

// This fragment is included inside a namespace.
	void  Fragment () { /* unchanged */ }

class Test
{
// methods
	ND_ bool	Init () __NE___;
		void	Done () __NE___;
};

void  Use (FILE* file DEBUG_ONLY( , Path filename)) __NE___
{
	DEBUG_ONLY( if ( file ) return );
	DEBUG_ONLY(
		if (file) { return; }
	);
	if (file) {}{}
	if (file) return;		else return;
}

template <typename ...Args>
void  Pack (Args&& ...args) { Call(args...); }

const char* raw = R"tag(
#define NOT_A_DIRECTIVE( x ) \
	(x)
)tag";

const char* unicode = "Привет";	// keep this gap

void  UseOperators ()
{
	Helper_t::OnDeallocate( ptr );
	::operator delete( ptr );
	object.operator->();
	auto value = item * 2u;
	auto member = object.*field;
	if (ready) return;	else		return;
	template_call<typename	Type>();
}
