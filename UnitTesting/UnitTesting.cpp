#include "UnitTesting.h"
#include <chrono>

namespace UnitTesting
{

///////////////////////////////////////////////////////////////////////////////

ResultObject::ResultObject() :
	m_TotalResult(Result::Success)
{}

Result ResultObject::GetTotalResult() const
{
	return m_TotalResult;
}

void ResultObject::AddResult(Result result)
{
	if(m_TotalResult != Result::Fail && result != Result::Success)
		m_TotalResult = result;
}

///////////////////////////////////////////////////////////////////////////////

Info::Info() :
	env(nullptr),
	suite(nullptr),
	test(nullptr),
	m_Name(""),
	m_File(""),
	m_Line(0)
{}

Info::Info(const std::string& name, const std::string& file, int line) :
	env(nullptr),
	suite(nullptr),
	test(nullptr),
	m_Name(name),
	m_File(file),
	m_Line(line)
{}

const std::string& Info::GetName() const
{
	return m_Name;
}
const std::string& Info::GetFile() const
{
	if(m_File.empty()) {
		if(test && !test->GetInfo().m_File.empty())
			return test->GetInfo().m_File;
		if(suite && !suite->GetInfo().m_File.empty())
			return suite->GetInfo().m_File;
	}

	return m_File;
}
int Info::GetLine() const
{
	return m_Line;
}

///////////////////////////////////////////////////////////////////////////////

AssertResult::AssertResult()
{}

AssertResult::AssertResult(
		const Info& inf, Result res, const std::string& msg) :
	message(msg),
	info(inf)
{
	ResultObject::AddResult(res);
}

///////////////////////////////////////////////////////////////////////////////

TestContext::TestContext(TestResult& r) :
	m_Results(r)
{}

void TestContext::AddResult(const Info& info,
		bool result, const std::string& msg)
{
	Info i = info;
	i.test = &m_Results.GetTest();
	i.suite = &i.test->GetSuite();
	i.env = &i.suite->GetEnvironment();
	AssertResult assertResult(info, result?Result::Success:Result::Fail, msg);
	i.test->GetSuite().GetEnvironment().GetControl()->OnAssert(assertResult);
	m_Results.AddResult(assertResult);
}

///////////////////////////////////////////////////////////////////////////////

TestResult::TestResult(Test* test) :
	m_Test(test),
	m_Milliseconds(0)
{}

void TestResult::AddResult(const AssertResult& result)
{
	m_Results.push_back(result);
	ResultObject::AddResult(result.GetTotalResult());
}

size_t TestResult::GetAssertCount() const
{
	return m_Results.size();
}

const AssertResult& TestResult::GetAssert(size_t i) const
{
	return m_Results[i];
}

const Test& TestResult::GetTest() const
{
	return *m_Test;
}

void TestResult::SetTime(double t)
{
	m_Milliseconds = t;
}

double TestResult::GetMilliseconds() const
{
	return m_Milliseconds;
}

///////////////////////////////////////////////////////////////////////////////

Test::Test(Suite& s, TestFunction func, const Info& info) :
	m_Func(func),
	m_Info(info)
{
	m_Info.test = this;
	m_Info.suite = &s;
	m_Info.env = &m_Info.suite->GetEnvironment();
	
	s.RegisterTest(this);
}

bool Test::Run(TestResult& result)
{
	TestContext ctx(result);
	try {
		auto begin = std::chrono::high_resolution_clock::now();
		m_Func(ctx);
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::ratio<1, 1000>> elapsedMillis = end - begin;

		result.SetTime(elapsedMillis.count());
	} catch(...) {
		ControlCallback* callback =
				m_Info.suite->GetEnvironment().GetControl();
		ControlAction action = callback->OnException(GetInfo());
		if(action == ControlAction::Abort)
			return false;
		if(action == ControlAction::AbortCurrent)
			return true;
		if(action == ControlAction::Ignore)
			(void)0;
		if(action == ControlAction::Procceed)
			result.AddResult(AssertResult(GetInfo(),
					Result::Fail, "Unknown Exception was thrown."));
	}

	return true;
}

const Info& Test::GetInfo() const
{
	return m_Info;
}

const Suite& Test::GetSuite() const
{
	return *m_Info.suite;
}

///////////////////////////////////////////////////////////////////////////////

SuiteResult::SuiteResult(Suite* suite) :
	m_Suite(suite)
{}

void SuiteResult::AddResult(const TestResult& result)
{
	m_Results.push_back(result);
	ResultObject::AddResult(result.GetTotalResult());
}

size_t SuiteResult::GetResultCount() const
{
	return m_Results.size();
}

const TestResult& SuiteResult::GetResult(size_t i) const
{
	return m_Results[i];
}

const TestResult& SuiteResult::GetResult(const std::string& name) const
{
	for(auto it = m_Results.begin(); it != m_Results.end(); ++it) {
		if(it->GetTest().GetInfo().GetName() == name)
			return *it;
	}

	static TestResult NULL_Result(nullptr);
	return NULL_Result;
}

const Suite& SuiteResult::GetSuite() const
{
	return *m_Suite;
}

void SuiteResult::SetTotalResult(Result result)
{
	m_TotalResult = result;
}

///////////////////////////////////////////////////////////////////////////////

SuiteFunction::SuiteFunction() :
	m_Func(nullptr)
{}

SuiteFunction::SuiteFunction(void(*f)(), const Info& info) :
	m_Func(f), m_Info(info)
{}

const Info& SuiteFunction::GetInfo() const
{
	return m_Info;
}

void SuiteFunction::operator()() const
{
	if(m_Func)
		m_Func();
}

///////////////////////////////////////////////////////////////////////////////

Suite::Suite(Environment& env, const Info& info) :
	m_Info(info)
{
	m_Info.env = &env;
	m_Info.suite = this;
	env.RegisterSuite(this);
}

bool Suite::Run(SuiteResult& result)
{
	bool procceed;
	if(!ExecFunction(m_Init, procceed))
		return procceed;
	
	for(auto it = m_Tests.begin(); it != m_Tests.end(); ++it) {
		GetEnvironment().GetControl()->OnTestBegin(**it);
		TestResult testResult(*it);
		do {
			testResult = TestResult(*it);
			if(!ExecFunction(m_Enter, procceed))
				return procceed;
		
			procceed = (*it)->Run(testResult);
			if(!procceed)
				return procceed;

			if(!ExecFunction(m_Leave, procceed))
				return procceed;
		} while(GetEnvironment().GetControl()->OnTestEnd(testResult));

		result.AddResult(testResult);
	}

	if(!ExecFunction(m_Exit, procceed))
		return procceed;

	return true;
}

const std::string& Suite::GetDependency(int i) const
{
	return m_Dependencies[i];
}

size_t Suite::GetDependencyCount() const
{
	return m_Dependencies.size();
}

const Test& Suite::GetTest(int i) const
{
	return *m_Tests[i];
}

size_t Suite::GetTestCount() const
{
	return m_Tests.size();
}

const Environment& Suite::GetEnvironment() const
{
	return *m_Info.env;
}

void Suite::RegisterTest(Test* t)
{
	m_Tests.push_back(t);
}

void Suite::RegisterDependency(const std::string& name)
{
	m_Dependencies.push_back(name);		
}

bool Suite::ExecFunction(const SuiteFunction& func, bool& procceed)
{
	try {
		func();
	} catch(...) {
		 ControlAction action = 
				m_Info.env->GetControl()->OnException(func.GetInfo());
		 if(action == ControlAction::Ignore || action == ControlAction::Procceed)
			 (void)0;
		 else if(action == ControlAction::AbortCurrent) {
			 procceed = true;
			 return false;
		 } else {
			 procceed = false;
			 return false;
		 }
	}

	return true;
}

const Info& Suite::GetInfo() const
{
	return m_Info;
}

bool Suite::CheckTag(const std::string& tag) const
{
	return (m_Tags.find(tag) != m_Tags.end());
}

void Suite::RegisterInit(void (*func)(), Info info)
{
	info.env = &GetEnvironment();
	info.suite = this;
	m_Init = SuiteFunction(func, info);
}

void Suite::RegisterExit(void (*func)(), Info info)
{
	info.env = &GetEnvironment();
	info.suite = this;
	m_Exit = SuiteFunction(func, info);
}

void Suite::RegisterFixtureEnter(void (*func)(), Info info)
{
	info.env = &GetEnvironment();
	info.suite = this;
	m_Enter = SuiteFunction(func, info);
}

void Suite::RegisterFixtureLeave(void (*func)(), Info info)
{
	info.env = &GetEnvironment();
	info.suite = this;
	m_Leave = SuiteFunction(func, info);
}

void Suite::AddTag(const std::string& tag)
{
	m_Tags.insert(tag);
}

///////////////////////////////////////////////////////////////////////////////

EnvironmentResult::EnvironmentResult(Environment* env) :
	m_Environment(env)
{}

void EnvironmentResult::AddResult(const SuiteResult& result)
{
	m_Results.push_back(result);
	ResultObject::AddResult(result.GetTotalResult());
}

size_t EnvironmentResult::GetResultCount() const
{
	return m_Results.size();
}

const SuiteResult& EnvironmentResult::GetResult(size_t i) const
{
	return m_Results[i];
}

const SuiteResult& EnvironmentResult::GetResult(const std::string& name) const
{
	for(auto it = m_Results.begin(); it != m_Results.end(); ++it) {
		if(it->GetSuite().GetInfo().GetName() == name) {
			return *it;
		}
	}

	static SuiteResult NULL_Result(nullptr);
	return NULL_Result;
}

const Environment& EnvironmentResult::GetEnvironment() const
{
	return *m_Environment;
}

///////////////////////////////////////////////////////////////////////////////

Environment::Environment() :
	m_Callback(nullptr)
{}

Environment& Environment::Instance()
{
	static Environment theInstance;
	return theInstance;
}

void Environment::SetControl(ControlCallback* control)
{
	m_Callback = control;
}

ControlCallback* Environment::GetControl() const
{
	return m_Callback;
}

void Environment::AddFilter(Filter* filter)
{
	m_Filter.push_back(filter);
}

void Environment::RemoveFilter(Filter* filter)
{
	for(auto it = m_Filter.begin(); it != m_Filter.end(); ++it) {
		if(*it == filter) {
			m_Filter.erase(it);
			return;
		}
	}
}

bool Environment::AllowSuite(const Suite* s) const
{
	for(auto it = m_Filter.begin(); it != m_Filter.end(); ++it) {
		if(!(*it)->IsOK(*s))
			return false;
	}

	return true;
}

size_t Environment::GetSuiteCount() const
{
	return m_Suites.size();
}

const Suite& Environment::GetSuite(size_t i) const
{
	return *m_Suites[i];
}

void Environment::RegisterSuite(Suite* suite)
{
	m_SuiteMap.emplace(suite->GetInfo().GetName(), m_Suites.size());
	m_Suites.push_back(suite);
}

bool Environment::CheckDependencies(const Suite* s,
		EnvironmentResult& result, bool& Procceed,
		const std::vector<size_t>& resultConnector)
{
	for(size_t i = 0; i != s->GetDependencyCount(); ++i){
		size_t suiteID = m_SuiteMap[s->GetDependency(i)];
		size_t resultID = resultConnector[suiteID];
		const SuiteResult& suiteRes = result.GetResult(resultID);
		if(suiteRes.GetTotalResult() != Result::Success) {
			ControlAction action = GetControl()->OnDependencyFail(
					*s, suiteRes.GetSuite(), suiteRes);
			if(action == ControlAction::Ignore) {
				(void)0;
			} else if(action == ControlAction::AbortCurrent) {
				Procceed = true;
				return false;
			} else {
				Procceed = false;
				return false;
			}
		}
	}

	return true;
}

bool Environment::RunSuites(const std::vector<Suite*>& suites,
		EnvironmentResult& result)
{
	std::vector<size_t> resultConnector(m_Suites.size());

	for(auto it= suites.begin(); it != suites.end(); ++it) {
		bool procceed = true;

		SuiteResult suiteResult(*it);
		GetControl()->OnSuiteBegin(**it);

		if(CheckDependencies(*it, result, procceed, resultConnector)) {
			if(!(*it)->Run(suiteResult))
				procceed = false;
		} else {
			suiteResult.SetTotalResult(Result::Unknown);
		}

		GetControl()->OnSuiteEnd(suiteResult);
		
		size_t suiteID = m_SuiteMap[(*it)->GetInfo().GetName()];
		size_t resultID = result.GetResultCount();
		resultConnector[suiteID] = resultID;

		result.AddResult(suiteResult);

		if(!procceed)
			return false;
	}

	return true;
}

int Environment::TopoVisit(size_t cur, std::vector<Suite*>& result,
		std::vector<std::pair<bool, bool>>& mark,
		std::vector<const Suite*>& unsolvable)
{
	if(mark[cur].second)
		return 1;

	if(mark[cur].first)
		return 0;

	mark[cur].second = true;
	for(size_t j = 0; j < m_Suites[cur]->GetDependencyCount(); ++j) {
		if(!mark[cur].first) {
			const std::string& depName = m_Suites[cur]->GetDependency(j);
			auto dep = m_SuiteMap.find(depName);
			if(dep == m_SuiteMap.end()) {
				ControlAction action = GetControl()->OnUnknownDependency(
						*m_Suites[cur], depName);
				if(action == ControlAction::Ignore)
					(void)0;
				else
					return 2;
			}

			int visitResult = TopoVisit(dep->second, result, mark, unsolvable);
			if(visitResult) {
				if(visitResult == 1)
					unsolvable.push_back(m_Suites[dep->second]);
				return visitResult;
			}
		}
	}

	mark[cur].first = true;
	mark[cur].second = false;
	result.push_back(m_Suites[cur]);

	return 0;
}

bool Environment::SolveDependencies(std::vector<Suite*>& result)
{
	std::vector<std::pair<bool, bool>> mark(m_Suites.size(),
			std::pair<bool,bool>(false, false));
	std::vector<const Suite*> unsolvable;

	bool succeded = true;
	for(size_t i = 0; i < m_Suites.size(); ++i) {
		if(!mark[i].first && AllowSuite(m_Suites[i])) {
			succeded = (TopoVisit(i, result, mark, unsolvable) == 0);
			if(!succeded)
				break;
		}
	}

	if(!succeded) {
		result.clear();
		if(!unsolvable.empty())
			GetControl()->OnUnsolvableDependencies(*this, unsolvable);
	}

	return succeded;
}

void Environment::Run()
{
	ConsoleCallback fallbackCallback;

	if(!m_Callback)
		m_Callback = &fallbackCallback;

	EnvironmentResult result(this);
	std::vector<Suite*> performSuites;

	if(SolveDependencies(performSuites)) {
		GetControl()->OnBegin(*this);
		RunSuites(performSuites, result);
		GetControl()->OnEnd(result);
	}

	if(m_Callback == &fallbackCallback)
		m_Callback = nullptr;
}

}