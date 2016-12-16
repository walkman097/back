#include "opds.h"
#include "ddv5entryview.h"
#include "ddv5stackview.h"
#include "testddv5entryview.h"

#include <QDebug>
#include <QStringList>
#include <cppunit/config/SourcePrefix.h>

CPPUNIT_TEST_SUITE_REGISTRATION( EntryViewTest );

void EntryViewTest::testButtonList()
{
	QStringList error;
	error << "bookInfo" << "CopyRight" << "Chapter" << "Comment" << "fuck";
	dangdangv5::ButtonArrayLabel *label = new dangdangv5::ButtonArrayLabel(error);
	label->show();
}

void EntryViewTest::testConentView()
{
	dangdangv5::ContentView *contentView = new dangdangv5::ContentView;
	opds::Entry entry;
	entry.id = "1900564496";
	entry.title = QString::fromUtf8("梦幻花");
	entry.subTitle = "1900564496";
	contentView->loadEntry(entry);
	contentView->showFullScreen();
}

void EntryViewTest::testCommentView()
{
#if 0
	dangdangv5::CommentView *commentView = new dangdangv5::CommentView;
	opds::Entry entry;
	entry.id = "1900564496";
	entry.title = QString::fromUtf8("梦幻花");
	entry.subTitle = "1900564496";
	opds::Link link;
	link.type = opds::IMAGE_JPEG;
	link.rel = opds::REL_THUMBNAIL;
	link.href = "http://img61.ddimg.cn/digital/product/44/96/1900564496_ii_cover.jpg?version=1a2209c1-662e-4a09-af6e-11c7f6422f3c";
	entry.links.push_back(link);
	commentView->loadEntry(entry);
	commentView->showFullScreen();
#endif
}

void EntryViewTest::testBookDetailView()
{
}

void EntryViewTest::setUp()
{
}

void EntryViewTest::tearDown()
{
}


