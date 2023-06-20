#include "pch.h"
#include <cassert>
#include <chrono>
#include <future>
#include <stdexcept>
#include <Windows.h>
#include "CppUnitTest.h"

//
using namespace Microsoft::VisualStudio::CppUnitTestFramework;


class AtomicityChecker
{
public:
	AtomicityChecker()
	{
		if (m_threadId) {
			throw std::runtime_error("The other thread intruded during invocation");
		}
		m_threadId = ::GetCurrentThreadId();
	}

	~AtomicityChecker()
	{
		m_threadId = {};
	}

private:
	static inline DWORD m_threadId;
};

#define ATOMICITY_CHECKER auto _ = AtomicityChecker()


void goo()
{
	using namespace std::chrono_literals;

	ATOMICITY_CHECKER;

	std::this_thread::sleep_for(1s);
}


void foo()
{
	for (auto i = 0; i < 100; ++i) {
		goo();
	}
}


void bar()
{
	for (auto i = 0; i < 100; ++i) {
		goo();
	}
}


void test()
{	
	auto f0 = std::async(std::launch::async, foo);
	auto f1 = std::async(std::launch::async, bar);
	f0.get();
	f1.get();
};


template<bool flag, typename T1, typename T2>
struct conditional
{
	constexpr static auto GetType()
	{
		if constexpr (flag == true) {
			return T1{};
		}
		else {
			return T2{};
		}
	}

	using type = decltype(GetType());

};


namespace UnitTest
{
	TEST_CLASS(UnitTest1)
	{
		TEST_METHOD(TestMethod1)
		{
			auto i = conditional<true, int, double>::type{};
			auto j = conditional<false, int, double>::type{};

			Assert::IsTrue(true);

			//Assert::ExpectException<std::runtime_error>(test);
		}
	};
}
