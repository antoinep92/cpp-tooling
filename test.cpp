#include <exception>
#include <string>
#include <sstream>
#include <iostream>

namespace test {

void make_string_(std::stringstream &) {}

template<class F, class... T>
void make_string_(std::stringstream & ss, F && first, T && ... next) {
	ss << first;
	make_string_(ss, next...);
}

template<class... T>
std::string && make_string(T && ... args) {
	std::stringstream ss;
	make_string_(ss, args...);
	return std::move(ss.str());
}

struct out_of_bounds : std::exception {
	size_t input;
	size_t bound;
	out_of_bounds(size_t input, size_t bound) : input(input), bound(bound) {}
	const char* what() const noexcept override { return make_string("out of bounds access at ", input, " where bound was ", bound).c_str(); }
};

template<class T> T max(T a, T b) { return a > b ? a : b; }
template<class T> T min(T a, T b) { return a < b ? a : b; }

// globals only used in checked_array should be moved there as statics
int array_counter = 0;
int index_counter = 0;

template<class T> struct checked_array {
	typedef T value_type;
	size_t size() const { return n; }

	size_t n;
	T* d;
	checked_array(size_t n = 0) : n(n), d(n ? new T[n] : 0) { ++array_counter; }
	checked_array(const checked_array<T> & cp) : n(cp.n), d(n ? new T[n] : 0) {
		if(n) memcpy(d, cp.d, sizeof(T) * n);
		++array_counter;
	}
	checked_array(checked_array<T> && mv) : n(mv.n), d(mv.d) {
		mv.d = 0; mv.n = 0;
		++array_counter;
	}
	~checked_array() { free(); --array_counter; }
	void free() { if(d) delete[] d; }
	void clear() { free(); d = 0; n = 0; }
	void resize(size_t nn) {
		T* nd;
		if(nn) {
			nd = new T[nn];
			if(n) memcpy(nd, d, sizeof(T) * max(n, nn));
		}
		free();
		d = nd; n = nn;
	}
	T & operator[](size_t i) {
		if(i >= n) throw out_of_bounds(i, n);
		++index_counter;
		return d[i];
	}
	T & operator[](size_t i) const {
		if(i >= n) throw out_of_bounds(i, n);
		++index_counter;
		return d[i];
	}
};

int x = 3; // scope test (global)

// dead class
struct independant {
	int x; // scope test (member)
	int f() { return x; }
	const char* g() { return "indep"; }
};

struct base {
	int x; // scope test (shadowed member)
	virtual int f() { return x; }
	base(int x = 0) : x(x) {}
};

struct derived : base {
	int x; // scope test (shadowing member)
	int f() { return base::x + x; }
	const char* g() { return "derived"; }
	derived(int x = 0, int base_x = 0) : base(base_x), x(x) {}
};

template<class F> checked_array<int> && build(size_t n, F f) {
	checked_array<int> arr(n);
	for(size_t i = 0; i < n; ++i) arr[i] = f();
	return std::move(arr);
}

template<class T> typename T::value_type sum(const T & vec) {
	typename T::value_type v = 0;
	for(size_t i = 0; i < vec.size(); ++i) v += vec[i];
	return v;
}

int test() {
	auto data = build(rand() % 100, [](){ return derived(rand() & 1, rand() & 1).f(); } );
	std::cout << sum(data) << std::endl;
	return 0;
}

} // test namespace
