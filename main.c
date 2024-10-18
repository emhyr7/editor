typedef struct
{
	int x, y;
} foo;

void do_foo(foo *foo)
{
}

void f() {}
void g() {}

int main()
{
	0 ? f() : g();
	do_foo(&(foo){0, 0});
}
