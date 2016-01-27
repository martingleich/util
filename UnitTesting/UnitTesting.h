#ifndef INCLUDED_UNITTESTING_H
#define INCLUDED_UNITTESTING_H
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <set>

namespace UnitTesting
{

class Environment;
class Suite;
class Test;
class TestResult;
class TestContext;
class Filter;
class ControlCallback;
typedef void (*TestFunction)(TestContext&);

struct Info
{
	Info();
	Info(const std::string& name, const std::string& file, int line);

	const std::string& GetName() const;
	const std::string& GetFile() const;
	int GetLine() const;

	const Environment* env;
	const Suite* suite;
	const Test* test;

private:
	std::string m_Name;
	std::string m_File;
	int m_Line;
};

enum class Result
{
	Success,
	Fail,
	Unknown
};

enum class ControlAction
{
	Procceed,
	Ignore,
	Abort,
	AbortCurrent,
	Repeat
};

class ResultObject
{
public:
	ResultObject();
	Result GetTotalResult() const;

protected:
	void AddResult(Result result);

protected:
	Result m_TotalResult;
};

struct AssertResult : public ResultObject
{
	std::string message;
	Info info;

	AssertResult();
	AssertResult(const Info& inf, Result res, const std::string& msg);
};

class TestContext
{
public:
	TestContext(TestResult& r);
	void AddResult(const Info& info, bool result, const std::string& msg);

private:
	TestResult& m_Results;
};

class TestResult : public ResultObject
{
public:
	TestResult(Test* test);

	void AddResult(const AssertResult& result);
	void SetTime(double t);

	size_t GetAssertCount() const;
	const AssertResult& GetAssert(size_t i) const;
	const Test& GetTest() const;
	double GetMilliseconds() const;

private:
	Test* m_Test;
	std::vector<AssertResult> m_Results;
	double m_Milliseconds;
};

class Test
{
friend class Suite;
public:
	Test(Suite& s, TestFunction func, const Info& info);
	const Info& GetInfo() const;
	const Suite& GetSuite() const;

private:
	bool Run(TestResult& result);

private:
	TestFunction m_Func;
	Info m_Info;
};

class SuiteResult : public ResultObject
{
public:
	SuiteResult(Suite* suite);
	void AddResult(const TestResult& result);
	size_t GetResultCount() const;
	const TestResult& GetResult(size_t i) const;
	const TestResult& GetResult(const std::string& name) const;
	const Suite& GetSuite() const;
	void SetTotalResult(Result result);

private:
	Suite* m_Suite;
	std::vector<TestResult> m_Results;
};

class SuiteFunction
{
public:
	SuiteFunction();
	SuiteFunction(void(*f)(), const Info& info);
	const Info& GetInfo() const;
	void operator()() const;

private:
	void(*m_Func)();
	Info m_Info;
};

class Suite
{
friend struct RegisterInit;
friend struct RegisterExit;
friend struct RegisterFixtureEnter;
friend class Test;
friend struct RegisterDependency;
friend struct AddTag;
friend struct RegisterFixtureLeave;
friend class Environment;
public:
	Suite(Environment& env, const Info& info);
	const std::string& GetDependency(int i) const;
	size_t GetDependencyCount() const;
	const Test& GetTest(int i) const;
	size_t GetTestCount() const;
	const Environment& GetEnvironment() const;
	const Info& GetInfo() const;
	bool CheckTag(const std::string& tag) const;

private:
	bool Run(SuiteResult& result);
	void RegisterInit(void (*func)(), Info info);
	void RegisterExit(void (*func)(), Info info);
	void RegisterFixtureEnter(void (*func)(), Info info);
	void RegisterFixtureLeave(void (*func)(), Info info);
	void RegisterTest(Test* t);
	void RegisterDependency(const std::string& name);
	void AddTag(const std::string& tag);
	bool ExecFunction(const SuiteFunction& func, bool& procceed);

private:
	SuiteFunction m_Init;
	SuiteFunction m_Enter;
	SuiteFunction m_Leave;
	SuiteFunction m_Exit;

	Info m_Info;

	std::vector<std::string> m_Dependencies;
	std::vector<Test*> m_Tests;
	std::set<std::string> m_Tags;
};

class EnvironmentResult : public ResultObject
{
public:
	EnvironmentResult(Environment* env);
	void AddResult(const SuiteResult& result);
	size_t GetResultCount() const;
	const SuiteResult& GetResult(size_t i) const;
	const SuiteResult& GetResult(const std::string& name) const;
	const Environment& GetEnvironment() const;

private:
	Environment* m_Environment;
	std::vector<SuiteResult> m_Results;
};

class Environment
{
friend class Suite;
public:
	Environment();
	static Environment& Instance();

	size_t GetSuiteCount() const;
	const Suite& GetSuite(size_t i) const;

	void SetControl(ControlCallback* control);
	ControlCallback* GetControl() const;

	void AddFilter(Filter* filter);
	void RemoveFilter(Filter* filter);

	void Run();

private:
	void RegisterSuite(Suite* suite);

private:
	bool CheckDependencies(const Suite* s, 
			EnvironmentResult& result, bool& Procceed,
			const std::vector<size_t>& resultConnector);
	bool RunSuites(const std::vector<Suite*>& suites, EnvironmentResult& result);
	int TopoVisit(size_t cur, std::vector<Suite*>& result, 
			std::vector<std::pair<bool, bool>>& mark,
			std::vector<const Suite*>& unsolvable);
	bool SolveDependencies(std::vector<Suite*>& result);
	bool AllowSuite(const Suite* s) const;

private:
	std::map<std::string, size_t> m_SuiteMap;
	std::vector<Suite*> m_Suites;

	ControlCallback* m_Callback;
	std::vector<Filter*> m_Filter;
};

struct RegisterInit
{
	RegisterInit(Suite& s, void(*f)(), const Info& info)
	{
		s.RegisterInit(f, info);
	}
};

struct RegisterExit
{
	RegisterExit(Suite& s, void(*f)(), const Info& info)
	{
		s.RegisterExit(f, info);
	}
};

struct RegisterDependency
{
	RegisterDependency(Suite& s, const std::string& name)
	{
		s.RegisterDependency(name);
	}
};

struct RegisterFixtureEnter
{
	RegisterFixtureEnter(Suite& s, void(*f)(), const Info& info)
	{
		s.RegisterFixtureEnter(f, info);
	}
};

struct RegisterFixtureLeave
{
	RegisterFixtureLeave(Suite& s, void(*f)(), const Info& info)
	{
		s.RegisterFixtureLeave(f, info);
	}
};

struct AddTag
{
	AddTag(Suite& s, const std::string& tag)
	{
		s.AddTag(tag);
	}
};

class ControlCallback
{
public:
	virtual ~ControlCallback() {}

	virtual void OnBegin(Environment& env) {}
	virtual void OnSuiteBegin(Suite& suite) {}
	virtual void OnTestBegin(Test& test) {}
	virtual void OnAssert(AssertResult& result) {}
	virtual bool OnTestEnd(const TestResult& result) { return false; }
	virtual void OnSuiteEnd(const SuiteResult& result) {}
	virtual void OnEnd(const EnvironmentResult& result) {}

	virtual ControlAction OnException(const Info& ctx) = 0;
	virtual ControlAction OnDependencyFail(const Suite& running,
			const Suite& failed, const SuiteResult& result) = 0;
	virtual ControlAction OnUnknownDependency(
			const Suite& from, const std::string& name) = 0;
	virtual ControlAction OnUnsolvableDependencies(
			const Environment& env, const std::vector<const Suite*>& unsolvable) = 0;
};

class Filter
{
public:
	virtual bool IsOK(const Suite& suite) = 0;
	virtual bool IsOK(const Test& test) = 0;
};

class ConsoleCallback : public ControlCallback
{
public:
	virtual ~ConsoleCallback() {}

	virtual void OnTestBegin(Test& t)
	{		
		std::cout << "   " << t.GetInfo().GetName() << " --> ";
	}

	virtual bool OnTestEnd(const TestResult& t)
	{
		if(t.GetTotalResult() == Result::Success)
			std::cout << "Succeeded";
		else if(t.GetTotalResult() == Result::Fail)
			std::cout << "Failed";

		std::cout << std::endl;

		if(t.GetTotalResult() == Result::Fail)
		{
			for(size_t i = 0; i < t.GetAssertCount(); ++i)
			{
				const AssertResult& a = t.GetAssert(i);
				std::cout << "     \"" << a.message << "\" --> ";
				if(a.GetTotalResult() == Result::Success)
					std::cout << "Succeeded";
				else if(a.GetTotalResult() == Result::Fail)
					std::cout << "Failed";

				std::cout << std::endl;
			}
		}

		return false;
	}

	virtual void OnSuiteBegin(Suite& s)
	{
		std::cout << "Run Testsuite \"" << s.GetInfo().GetName() << "\":" << std::endl;
	}

	virtual void OnSuiteEnd(const SuiteResult& result)
	{
		std::cout << std::endl;
	}

	virtual void OnBegin(Environment& env)
	{}
	virtual void OnEnd(const Environment& env)
	{}

	virtual ControlAction OnException(const Info& ctx)
	{
		std::cout << "Unknown exception was thrown." << std::endl;
		return ControlAction::AbortCurrent;
	}
	virtual ControlAction OnDependencyFail(const Suite& running,
			const Suite& failed, const SuiteResult& result)
	{
		std::cout << "Dependency \"" << failed.GetInfo().GetName() <<
			"\" needed by \"" << running.GetInfo().GetName() << "\" failed." << std::endl;
		return ControlAction::AbortCurrent;
	}
	virtual ControlAction OnUnknownDependency(
			const Suite& s, const std::string& dep)
	{
		std::cout << "Missing dependency \"" << dep << "\"." << std::endl;
		return ControlAction::AbortCurrent;
	}
	virtual ControlAction OnUnsolvableDependencies(
			const Environment& e, const std::vector<const Suite*>& unsolvable)
	{
		std::cout << "Can not solve dependencies, circle by:" << std::endl;
		for(auto it = unsolvable.begin(); it != unsolvable.end(); ++it)
			std::cout << "   " << (*it)->GetInfo().GetName() << std::endl;

		return ControlAction::Abort;
	}
};

}

#define UNIT_SUITE(name) \
namespace Suite_##name \
{ \
static UnitTesting::Suite Suite(UnitTesting::Environment::Instance(), UnitTesting::Info(#name, __FILE__, __LINE__)); \
} \
namespace Suite_##name

#define UNIT_SUITE_DEPEND_ON(dependency) \
static UnitTesting::RegisterDependency RegisterSuiteDepend_##dependency(Suite, #dependency);

#define UNIT_SUITE_TAG(tag) \
static UnitTesting::AddTag AddTag_##tag(Suite, #tag);

#define UNIT_TEST(name) \
void TestFunc_##name(UnitTesting::TestContext&); \
static UnitTesting::Test Test_##name(Suite, &TestFunc_##name, UnitTesting::Info(#name, "", __LINE__)); \
void TestFunc_##name(UnitTesting::TestContext& ctx)

#define UNIT_SUITE_INIT \
void SuiteInitFunc(); \
static UnitTesting::RegisterInit RegisterSuiteInit(Suite, &SuiteInitFunc, UnitTesting::Info("suite.init", "", __LINE__)); \
void SuiteInitFunc()

#define UNIT_SUITE_EXIT \
void SuiteExit(); \
static UnitTesting::RegisterExit RegisterSuiteExit(Suite, &SuiteExit, UnitTesting::Info("suite.exit", "", __LINE__)); \
void SuiteExit()

#define UNIT_SUITE_FIXTURE_ENTER \
void SuiteFixtureEnter(); \
static UnitTesting::RegisterFixtureEnter RegisterFixtureEnter(Suite, &SuiteFixtureEnter, UnitTesting::Info("suite.fixture_enter", "", __LINE__)); \
void SuiteFixtureEnter()

#define UNIT_SUITE_FIXTURE_LEAVE \
void SuiteFixtureLeave(); \
static UnitTesting::RegisterFixtureLeave RegisterFixtureLeave(Suite, &SuiteFixtureLeave, UnitTesting::Info("suite.fixture_leave", "", __LINE__)); \
void SuiteFixtureLeave()

#define UNIT_ASSERT(cond) do{ ctx.AddResult(UnitTesting::Info("", "", __LINE__), (cond), #cond); }while(false)
#define UNIT_ASSERT_EX(cond, msg) do{ ctx.AddResult(UnitTesting::Info("", "", __LINE__), (cond), (msg)); }while(false)

#endif