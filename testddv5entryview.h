#ifndef _TESTDDV5ENTRYVIEW_H_
#define _TESTDDV5ENTRYVIEW_H_

#include <cppunit/extensions/HelperMacros.h>


class EntryViewTest : public CPPUNIT_NS::TestFixture {
public:
	CPPUNIT_TEST_SUITE(EntryViewTest);
	//CPPUNIT_TEST(testButtonList);
	//CPPUNIT_TEST(testConentView);
	CPPUNIT_TEST(testCommentView);
	//CPPUNIT_TEST(testBookDetailView);
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp();
	void tearDown();

protected:
	void testButtonList();
	void testConentView();
	void testCommentView();
	void testBookDetailView();
};


#endif
