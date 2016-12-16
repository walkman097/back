#include "ddv5entrycommand.h"
#include "constants.h"
#include "feedview.h"
#include "bookstoreutil.h"
#include "bookstorefactory.h"
#include "ddv5entryrequest.h"
#include "ddv5entryview.h"
#include "ddv5commentmanager.h"
#include "ddv5commentrequest.h"
#include "thumbnailview.h"
#include "dangdang/dangdangutil.h"
#include "dangdang/ddentryview.h"
#include <QTimer>
#include <QUrl>

namespace dangdangv5 {
	
	//class BookInfoCommand
	BookInfoCommand::BookInfoCommand(dangdangv5::BookDetailView *v)
		:view(v)
	{
		QNetworkAccessManager *mgr = opds::BookStoreUtil::getInstance()->getNetworkMgr();
		request = new dangdangv5::BookInfoRequest(mgr, this);
		connect(request, SIGNAL(finished()), SLOT(finished()));
	}

	BookInfoCommand::~BookInfoCommand()
	{
		delete request;
	}

	void BookInfoCommand::execute(const opds::Entry &e)
	{
		request->setEntry(e);
		request->execute();
	}
	
	void BookInfoCommand::finished()
	{
		qDebug() << "BookInfoCommand" << __FILE__ << __func__ << "setDetailEntry begin!!!!!!!!";
		view->setDetailEntry(request->getEntry());
	}

	//class BookCommentCommand
	BookCommentCommand::BookCommentCommand(QObject *parent)
	{
		opds::BookStoreFactory *factory = opds::BookStoreUtil::getInstance()->factory();
		request = dynamic_cast<dangdangv5::CommentRequest *>(factory->getCommentManager()->getCommentRequest());
	}

	BookCommentCommand::~BookCommentCommand()
	{
	}
	
	void BookCommentCommand::execute(const opds::Entry &e)
	{
		request->setEntry(e);
		request->execute();
	}

	//class AlsoBuyCommand
	AlsoBuyCommand::AlsoBuyCommand(opds::ThumbnailView *view)
	{
		clear();
		QNetworkAccessManager *mgr = opds::BookStoreUtil::getInstance()->getNetworkMgr();		
		request = new dangdangv5::AlsoBuyRequest(mgr, this);
		connect(request, SIGNAL(finished()), SLOT(finishedSlot()));
		request->setAction("recommendBase");
		view->setRequest(request);
	}

	AlsoBuyCommand::~AlsoBuyCommand()
	{
		delete request;
		request = NULL;
	}
	
	void AlsoBuyCommand::clear()
	{
		total = 0;
		start = 0;
		end = 8;
	}

	void AlsoBuyCommand::refresh(const opds::Entry &e)
	{
		start = end + 1;
		end += 9;
		if (start >= total)
			clear();
		setUrl(e);
		request->execute();
	}
		
	void AlsoBuyCommand::setUrl(const opds::Entry &entry)
	{
		opds::Link link;
		link.type = opds::NAVIGATION;
		link.rel = opds::REL_START;
		link.userData[dangdang::Util::ACTION] = "recommendBase";
		if (entry.subTitle != entry.id && !entry.subTitle.isEmpty()) {
			link.userData["mediaId"] = entry.subTitle;
		} else {
			link.userData["mediaId"] = entry.id;
		}
		link.userData["position"] = "2";
		link.userData["platformSource"] = "DDDS-P";
		link.userData["channelType"] = "";
	
		QUrl url = dangdang::Util::getInstance()->getUrl(link, request);
		url.removeQueryItem("start");
		url.removeQueryItem("end");
		url.removeQueryItem("type");
		url.addQueryItem("start", QString::number(start));
		url.addQueryItem("end", QString::number(end));
		request->setUrl(url.toString());
		request->clearFeed();
	}

	void AlsoBuyCommand::execute(const opds::Entry &e)
	{
		clear();
		setUrl(e);
		request->execute();
	}
	
	void AlsoBuyCommand::finishedSlot()
	{
		int code = request->getResultCode();
		qDebug() << "AlsoBuyCommand" << __func__ << "code" << code; 
		if (code == netview::Parser::SUCCESS) {
			const opds::Feed &currentFeed = request->getFeed();
			total = currentFeed.totalResults; 
		}
	}

}


