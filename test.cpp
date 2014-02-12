struct independant {
	int x;
	void f();
	const char* g() { return "indep"; }
};

struct base {
	virtual void f() = 0;
};

struct derived : base {
	int x;
	void f() { return x; }
	const char* g() { return "derived"; }
};

int main() {
	return 0;
}
