#ifndef TESTDDCLOUDWIDGET_H
#define TESTDDCLOUDWIDGET_H

#include <cppunit/extensions/HelperMacros.h>
#include <QObject>


class TestCloudShelf : public QObject, public CPPUNIT_NS::TestFixture {
	Q_OBJECT
public:
	CPPUNIT_TEST_SUITE(TestCloudShelf);
	CPPUNIT_TEST(testCloudShelfWidget);
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp();
	void tearDown();

protected:
	void testCloudShelfWidget();

};


#endif
