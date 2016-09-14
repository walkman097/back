#include "ddv5cloudrequest_p.h"
#include "ddv5cloudmanager.h"
#include "dangdang/dangdangutil.h"
#include "statuscodeparser.h"
#include "constants.h"
#include "opdsutil.h"
#include "opds.h"

#include <QDir>
#include <QFile>
#include <QDebug>
#include <QTimer>
#include <QFileInfo>
#include <QSettings>

namespace dangdangv5 {

	//class VipBookRequest
	VipBookRequest::VipBookRequest(const QString &path, QNetworkAccessManager *mgr, QObject *parent)
		:dangdangv5::LocalFileFeedRequest(mgr, parent)
	{
		setLoadOnce(false);
		QDir dir(path);
		if (!dir.exists()) {
			dir.mkpath(path);
		}

		saveFilePath = path + QDir::separator() + "vip.json";
		QFileInfo fi(saveFilePath);
		if (fi.size() <= 0)
			QFile::remove(saveFilePath);
		getParser()->clear();
		clearLocalFeed();
		loadFromFile(saveFilePath);
	}

	VipBookRequest::~VipBookRequest()
	{
	}

	const QString VipBookRequest::action() const 
	{
		return "getUserChannelBookList";
	}

	void VipBookRequest::execute()
	{
		opds::Link link; 	
		link.userData[dangdang::Util::ACTION] = "getUserChannelBookList";
		link.userData["platformSource"] = "DDDS-P";
		link.userData["channelType"] = "";
		QUrl url = dangdang::Util::getInstance()->getUrl(link, this);
		url.removeQueryItem("start");
		url.removeQueryItem("end");
		url.removeQueryItem("type");
		url.removeQueryItem("pageNum");
		url.removeQueryItem("pageSize");
		setUrl(url.toString());
		dangdang::LocalFileFeedRequest::execute();
	}
	
	netview::Parser* VipBookRequest::createParser()
	{
		return new dangdangv5::VIPBookParser;	
	}

	bool VipBookRequest::hasNext()
	{
		return false;
	}
	
	void VipBookRequest::responsed(QNetworkReply *r, netview::Parser *p)
	{
		dangdang::LocalFileFeedRequest::responsed(r, p);
		QStringList productIds;
		const opds::Feed &feed = getFeed();
		std::list<opds::Entry>::const_iterator it = feed.entryList.begin();
		for(; it != feed.entryList.end(); ++it) {
			productIds.push_back(it->id);
		}
		
		QSettings cfg("cloud");
		cfg.beginGroup("VIPBook");
		cfg.setValue("productIds", productIds);
		cfg.endGroup();
	}
	
	bool VipBookRequest::entryMatched(const opds::Entry &old, const opds::Entry &current)
	{
		bool ret = (old == current);
		if (!ret) 
			return false;
	
		std::list<opds::Link>::const_iterator it1 = old.links.begin();
		std::list<opds::Link>::const_iterator it2 = current.links.begin();
		for (;
		     it1 != old.links.end() && it2 != current.links.end();
		     ++it1, ++it2) {
			if (it1->type != it2->type ||
			    it1->userData != it2->userData ||
			    it1->rel != it2->rel) {
				opds::OPDSUtil::printEntry(old);
				opds::OPDSUtil::printEntry(current);
				return false;
			}
		}
		return true;
	}

	//class BorrowRequest
	BorrowRequest::BorrowRequest(const QString &path, QNetworkAccessManager *mgr, QObject *parent)
		:dangdangv5::LocalFileFeedRequest(mgr, parent)
	{
		setLoadOnce(false);
		QDir dir(path);
		if (!dir.exists()) {
			dir.mkpath(path);
		}

		saveFilePath = path + QDir::separator() + "borrow.json";
		QFileInfo fi(saveFilePath);
		if (fi.size() <= 0)
			QFile::remove(saveFilePath);
		getParser()->clear();
		clearLocalFeed();
		loadFromFile(saveFilePath);
	}

	BorrowRequest::~BorrowRequest()
	{
	}

	const QString BorrowRequest::action() const 
	{
		return "getUserChannelBookList";
	}

	void BorrowRequest::execute()
	{
		opds::Link link; 	
		link.userData[dangdang::Util::ACTION] = "getBorrowAuthorityList";
		link.userData["lastBorrowAuthorityId"] = "";
		link.userData["pageSize"] = "500";
		link.userData["platformSource"] = "DDDS-P";
		link.userData["channelType"] = "";
		QUrl url = dangdang::Util::getInstance()->getUrl(link, this);
		url.removeQueryItem("start");
		url.removeQueryItem("end");
		url.removeQueryItem("type");
		url.removeQueryItem("pageNum");
		setUrl(url.toString());
		dangdang::LocalFileFeedRequest::execute();
	}
	
	netview::Parser* BorrowRequest::createParser()
	{
		return new dangdangv5::BorrowBookParser;	
	}

	bool BorrowRequest::hasNext()
	{
		return false;
	}
	
	bool BorrowRequest::entryMatched(const opds::Entry &old, const opds::Entry &current)
	{
		bool ret = (old == current);
		if (!ret) 
			return false;
	
		std::list<opds::Link>::const_iterator it1 = old.links.begin();
		std::list<opds::Link>::const_iterator it2 = current.links.begin();
		for (;
		     it1 != old.links.end() && it2 != current.links.end();
		     ++it1, ++it2) {
			if (it1->type != it2->type ||
			    it1->userData != it2->userData ||
			    it1->rel != it2->rel) {
				opds::OPDSUtil::printEntry(old);
				opds::OPDSUtil::printEntry(current);
				return false;
			}
		}
		return true;
	}

	//class BuyRequest
	BuyRequest::BuyRequest(QNetworkAccessManager *mgr, const QString type, QObject *parent)
		:dangdangv5::LocalFileFeedRequest(mgr, parent)
	{
		doNext = false;
		this->type = type;
	}

	BuyRequest::~BuyRequest()
	{
	}

	const QString BuyRequest::action() const 
	{
		return "getUserChannelBookList";
	}
	
	void BuyRequest::reset()
	{
		lastAuthorityId.clear();
		doNext = false;
	}

	void BuyRequest::execute()
	{
		opds::Link link; 	
		link.userData[dangdang::Util::ACTION] = "getUserBookList";
		link.userData["lastMediaAuthorityId"] = lastAuthorityId;
		link.userData["lastRequestTime"] = "";
		link.userData["platformSource"] = "DDDS-P";
		link.userData["channelType"] = "";
		QUrl url = dangdang::Util::getInstance()->getUrl(link, this);
		url.removeQueryItem("start");
		url.removeQueryItem("end");
		url.removeQueryItem("pageNum");
		url.removeQueryItem("type");
		url.removeQueryItem("pageSize");
		url.addQueryItem("pageSize", "1000");
		setUrl(url.toString());
		dangdang::LocalFileFeedRequest::execute();
	}

	void BuyRequest::next()
	{
		BuyRequest::execute();
	}
	
	netview::Parser* BuyRequest::createParser()
	{
		return new dangdangv5::BuyBookParser(type);	
	}
	
	void BuyRequest::responsed(QNetworkReply *, netview::Parser *)
	{
		BuyBookParser *parser = dynamic_cast<BuyBookParser *>(getParser());
		if (parser) {
			qDebug() << "BuyRequest" << __func__ << "systemDate" << parser->getSystemDate();
			lastAuthorityId = parser->getMediaAuthorityId();
			doNext = (parser->getBuyBook().entryList.size() == 999);
		}
	}

	bool BuyRequest::hasNext()
	{
		return doNext;
	}
	
}


