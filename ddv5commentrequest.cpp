#include "ddv5commentrequest.h"
#include "ddv5commentparser.h"
#include "statuscodeparser.h"
#include "dangdang/dangdangutil.h"
#include "bookstoreutil.h"
#include "opdsutil.h"
#include "systemmanager.h"

#include <QDateTime>
#include <QDebug>

namespace dangdangv5 {
	
	//class CommentRequest
	CommentRequest::CommentRequest(
		QNetworkAccessManager *m, QObject *parent)
		:dangdangv5::FeedRequest(m, parent)
	{
	}

	CommentRequest::~CommentRequest()
	{
	}

	netview::Parser *CommentRequest::createParser()
	{
		return new dangdangv5::CommentParser;
	}

	void CommentRequest::setEntry(const opds::Entry &e)
	{
		qDebug() << "CommentRequest" << __func__ << "e.id" << e.id << "e.subTitle" << e.subTitle;
		opds::Link link;
		link.userData[dangdang::Util::ACTION] = "queryArticleListV2";
		if (e.subTitle.isEmpty()) {
			link.userData["objectId"] = e.id;
		} else {
			link.userData["objectId"] = e.subTitle;
		}
		setLink(link);
		barId = -1;
	}
	
	bool CommentRequest::hasNext()
	{
		return false;
	}

	int CommentRequest::getBarId()
	{
		return barId;	
	}
	
	int CommentRequest::getMemberStatus()
	{
		return memberStatus;	
	}

	void CommentRequest::responsed(QNetworkReply *r, netview::Parser *p)
	{
		FeedRequest::responsed(r, p);
		CommentParser *parser = dynamic_cast<CommentParser *>(getParser());
		if (parser) {
			barId = parser->getBarId();
			memberStatus = parser->getMemberStatus();
		}
	}

	//class MoreCommentRequest
	MoreCommentRequest::MoreCommentRequest(QNetworkAccessManager *mgr, QObject *parent)
		:CommentRequest(mgr, parent)
	{
	}

	MoreCommentRequest::~MoreCommentRequest()
	{
	}
	
	bool MoreCommentRequest::hasNext()
	{
		return hasMore;	
	}

	void MoreCommentRequest::setLastModifiDate(const QString &lastDate)
	{
		opds::Link link;
		link.userData[dangdang::Util::ACTION] = "queryArticleListV2";
		link.userData["objectId"] = objectId;
		link.userData["lastVisitDateMsec"] = QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch());
		link.userData["lastModifiedDateMsec"] = lastDate;
		link.userData["barId"] = barId; 
		QUrl url = dangdang::Util::getInstance()->getUrl(link, this);
		setUrl(url.toString());
		execute();
	}

	void MoreCommentRequest::next()
	{
		setLastModifiDate(lastModifiDate);
	}

	void MoreCommentRequest::reset()
	{
		lastModifiDate.clear();
		hasMore = false;
	}
	
	void MoreCommentRequest::responsed(QNetworkReply *, netview::Parser *)
	{
		CommentParser *parser = dynamic_cast<CommentParser *>(getParser());
		lastModifiDate = parser->getLastModifyDate();
		if (parser->getFeed().entryList.size() < 50)
			hasMore = false;
		else 
			hasMore = true;
	}

	void MoreCommentRequest::setEntry(const opds::Entry &e)
	{
		qDebug() << "MoreCommentRequest" << __func__ << "e.id" << e.id << "barId" 
			<< e.dcIndentifier; 
		opds::Link link;
		link.userData[dangdang::Util::ACTION] = "queryArticleListV2";
		if (e.subTitle.isEmpty()) {
			link.userData["objectId"] = e.id;
			objectId = e.id; 
		} else {
			link.userData["objectId"] = e.subTitle;
			objectId = e.subTitle;
		}
		link.userData["lastVisitDateMsec"] = "0";
		link.userData["barId"] = e.dcIndentifier;
		barId = e.dcIndentifier;
		setLink(link);
	}
	
	//class AddCommentRequest
	AddCommentRequest::AddCommentRequest(QNetworkAccessManager *mgr, QObject *parent)
		:netview::Request(mgr, parent)
	{
		setMethod(netview::Request::POST);
	}

	AddCommentRequest::~AddCommentRequest()
	{
	}
	
	void AddCommentRequest::fill(QNetworkRequest *r)
	{
		r->setRawHeader("Content-Type", "application/x-www-form-urlencoded");
	}

	void AddCommentRequest::setLink(const opds::Link &lnk)
	{
		QUrl url(opds::BookStoreUtil::getInstance()->getBasicUrl());
		url.addQueryItem(dangdang::Util::ACTION, "publishArticle");
		setUrl(url.toString());

		opds::Link link;
		link.userData["barId"] = lnk.userData.value("barId");
		link.userData["content"] = lnk.userData.value("content");
		link.userData["title"] = lnk.userData.value("title");
		link.userData["actionType"] = "1";
		link.userData["cardType"] = "0";
		QUrl dataUrl = dangdang::Util::getInstance()->getUrl(link, this);
		setPostData(dataUrl.encodedQuery());
	}

	void AddCommentRequest::responsed(QNetworkReply *, netview::Parser *)
	{
		opds::StatusCodeParser *parser = (opds::StatusCodeParser *)getParser();
		setResultCode(parser->getStatusCode());
		setResultMsg(parser->getErrorString());
	}
	
	netview::Parser* AddCommentRequest::createParser()
	{
		return new opds::StatusCodeParser; 
	}

	void AddCommentRequest::setResultCode(const int &val)
	{
		code = val;
	}

	void AddCommentRequest::setResultMsg(const QString &msg)
	{
		message = msg;
	}
	
	int AddCommentRequest::getResultCode()
	{
		return code;
	}

	const QString& AddCommentRequest::getResultMsg()
	{
		return message;	
	}

	AddBarRequest::AddBarRequest(QNetworkAccessManager *m, QObject *parent)
		:dangdangv5::AddCommentRequest(m, parent)
	{
		setMethod(netview::Request::POST);
	}
	
	AddBarRequest::~AddBarRequest()
	{
	}

	const QString AddBarRequest::action() const
	{
		return "barMember";
	}

	void AddBarRequest::setBarId(const int &id)
	{
		QUrl url(opds::BookStoreUtil::getInstance()->getBasicUrl());
		url.addQueryItem(dangdang::Util::ACTION, "barMember");
		url.addQueryItem("barId", QString::number(id));
		url.addQueryItem("actionType", "1");
		setUrl(url.toString());

		opds::Link link;
		link.userData["platformSource"] = "DDDS-P";
		link.userData["channelType"] = "";
		QUrl dataUrl = dangdang::Util::getInstance()->getUrl(link, this);
		setPostData(dataUrl.encodedQuery());
	}

	void AddBarRequest::responsed(QNetworkReply *, netview::Parser *)
	{
		opds::StatusCodeParser *parser = (opds::StatusCodeParser *)getParser();
		setResultCode(parser->getStatusCode());
		setResultMsg(parser->getErrorString());
	}
	
	netview::Parser* AddBarRequest::createParser()
	{
		return new opds::StatusCodeParser;	
	}

	void AddBarRequest::fill(QNetworkRequest *r)
	{
		r->setRawHeader("Content-Type", "application/x-www-form-urlencoded");
	}
	
	//class CommentInfoRequest
	CommentInfoRequest::CommentInfoRequest(QNetworkAccessManager *mgr, QObject *parent)	
		:AddCommentRequest(mgr, parent)
	{
		setMethod(netview::Request::POST);
	}

	CommentInfoRequest::~CommentInfoRequest()
	{
	}

	const QString CommentInfoRequest::action()
	{
		return "queryArticleInfoV2";
	}
	
	const opds::Entry CommentInfoRequest::getEntry()
	{
		return entry;
	}

	void CommentInfoRequest::setEntry(const opds::Entry &entry)
	{
		this->entry.clear();
		QUrl url(opds::BookStoreUtil::getInstance()->getBasicUrl());
		url.addQueryItem(dangdang::Util::ACTION, "queryArticleInfoV2");
		url.addQueryItem("mediaDigestId", entry.id);
		setUrl(url.toString());

		opds::Link link;
		link.userData["platformSource"] = "DDDS-P";
		link.userData["channelType"] = "";
		QUrl dataUrl = dangdang::Util::getInstance()->getUrl(link, this);
		setPostData(dataUrl.encodedQuery());
	}

	void CommentInfoRequest::responsed(QNetworkReply *, netview::Parser *)
	{
		CommentInfoParser *parser = dynamic_cast<CommentInfoParser *>(getParser());
		setResultCode(parser->getResultCode());
		setResultMsg(parser->getStatusMsg());
		entry = parser->getEntry();
	}
	
	netview::Parser* CommentInfoRequest::createParser()
	{
		return new CommentInfoParser;
	}

	void CommentInfoRequest::fill(QNetworkRequest *r)
	{
		r->setRawHeader("Content-Type", "application/x-www-form-urlencoded");
	}
	
	//class QueryBarListRequest 
	QueryBarListRequest::QueryBarListRequest(QNetworkAccessManager *mgr, QObject *parent)
		:dangdangv5::FeedRequest(mgr, parent)
	{
	}

	QueryBarListRequest::~QueryBarListRequest()
	{
	}
	
	void QueryBarListRequest::setBarName(const QString &name)
	{
		opds::Link link;
		link.userData[dangdang::Util::ACTION] = action();
		link.userData["barName"] = name;
		link.userData["platformSource"] = "DDDS-P";
		link.userData["channelType"] = "";
		setLink(link);
	}

	const QString QueryBarListRequest::action() const 
	{
		return "queryBarList";
	}
	
	bool QueryBarListRequest::isExist()
	{
		return exist;
	}

	void QueryBarListRequest::responsed(QNetworkReply *, netview::Parser *)
	{
		QueryBarListParser *parser = dynamic_cast<QueryBarListParser *>(getParser());
		exist = parser->isExist();
	}

	netview::Parser* QueryBarListRequest::createParser()
	{
		return new QueryBarListParser;
	}

	//class GetRecommendTagsRequest
	GetRecommendTagsRequest::GetRecommendTagsRequest(QNetworkAccessManager *mgr, QObject *parent)
		:dangdangv5::FeedRequest(mgr, parent)
	{
		opds::Link link;
		link.userData[dangdang::Util::ACTION] = action();
		link.userData["platformSource"] = "DDDS-P";
		link.userData["channelType"] = "";
		setLink(link);
	}

	GetRecommendTagsRequest::~GetRecommendTagsRequest()
	{
	}

	const QString GetRecommendTagsRequest::action() const 
	{
		return "getRecommendTags";
	}
	
	void GetRecommendTagsRequest::responsed(QNetworkReply *, netview::Parser *)
	{
	}

	netview::Parser* GetRecommendTagsRequest::createParser()
	{
		return new GetRecommendTagsParser;
	}
	

	//class CreateBarRequest
	CreateBarRequest::CreateBarRequest(QNetworkAccessManager *mgr, QObject *parent)
		:dangdangv5::FeedRequest(mgr, parent)
	{
	}

	CreateBarRequest::~CreateBarRequest()
	{
	}

	const QString CreateBarRequest::action() const 
	{
		return "createBar";
	}
	
	int CreateBarRequest::getStatusCode()
	{
		return code;
	}

	const QString CreateBarRequest::getErrorString()
	{
		return error;
	}

	void CreateBarRequest::setLink(const opds::Link &lnk)
	{
		opds::Link link;
		link.userData[dangdang::Util::ACTION] = action();
		link.userData["barName"] = lnk.userData.value("barName");
		link.userData["barDesc"] = lnk.userData.value("barDesc");
		link.userData["actionType"] = lnk.userData.value("actionType");;
		link.userData["bookType"] = lnk.userData.value("bookType");
		link.userData["objectId"] = lnk.userData.value("objectId");
		link.userData["tags"] = lnk.userData.value("tags");
		QUrl url = dangdang::Util::getInstance()->getUrl(link, this);
		url.removeQueryItem("end");
		url.removeQueryItem("type");
		url.removeQueryItem("start");
		url.removeQueryItem("pageNum");
		url.removeQueryItem("pageSize");
		url.removeQueryItem("textFieldType");
		QString encodeUrl = url.encodedQuery();
		encodeUrl.prepend(url.toString().toUtf8().left(url.toString().indexOf("?") + 1));
		setUrl(encodeUrl);
	}

	void CreateBarRequest::responsed(QNetworkReply *, netview::Parser *)
	{
		CreateBarParser *parser = dynamic_cast<CreateBarParser *>(getParser());
		code = parser->getStatusCode();
		error = parser->getErrorString();
	}

	netview::Parser* CreateBarRequest::createParser()
	{
		return new CreateBarParser;
	}

}

