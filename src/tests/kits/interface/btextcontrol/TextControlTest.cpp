#include "../common.h"

#include <Application.h>
#include <String.h>
#include <TextControl.h>

class TextControlTestcase: public TestCase {
public:
	void
	ClassSizeTest()
	{
#ifdef B_HAIKU_32_BIT
		CPPUNIT_ASSERT_EQUAL(312, sizeof(BTextControl));
#else
		CPPUNIT_ASSERT(true);
#endif
	}

	void
	GetTextTest()
	{
		BApplication app("application/x-vnd.Haiku-interfacekit-textcontroltest");
		BRect textRect(0, 0, 100, 100);
		BTextControl* v = new BTextControl(textRect, "test", 0, 0, 0);
		v->SetText("Initial text");
		v->TextView()->Insert(8, "(inserted) ", 10);
		CPPUNIT_ASSERT_EQUAL(BString("Initial (inserted)text"), v->Text());
	}
};


Test*
TextControlTestSuite()
{
	TestSuite *testSuite = new TestSuite();

	testSuite->addTest(new CppUnit::TestCaller<TextControlTestcase>(
		"BTextControl_ClassSize", &TextControlTestcase::ClassSizeTest));
	testSuite->addTest(new CppUnit::TestCaller<TextControlTestcase>(
		"BTextControl_GetText", &TextControlTestcase::GetTextTest));

	return testSuite;
}
