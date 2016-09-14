#include "testddcloudwidget.h"
#include "dangdangshelf/cloudfeedviewshelf.h"
#include "feedviewdelegate.h"
#include "bookstoreutil.h"
#include "cloudmanager.h"
#include "bookstorefactory.h"

#include <cppunit/config/SourcePrefix.h>
#include <QNetworkAccessManager>
#include <QDebug>


CPPUNIT_TEST_SUITE_REGISTRATION( TestCloudShelf);

void TestCloudShelf::testCloudShelfWidget()
{
	dangdang::CloudShelfWidget *shelf = new dangdang::CloudShelfWidget;
	shelf->load();
	shelf->showFullScreen();
}

void TestCloudShelf::setUp()
{
}

void TestCloudShelf::tearDown()
{	
}

