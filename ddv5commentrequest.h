#ifndef DDV5COMMENTREQUEST_H
#define DDV5COMMENTREQUEST_H

#include "opds.h"
#include "request.h"
#include "ddv5feedrequest.h"

class QNetworkAccessManager;

namespace dangdangv5 {
	
	//CommentRequest
	class CommentRequest : public dangdangv5::FeedRequest {
	public:
		CommentRequest(QNetworkAccessManager *m, QObject *parent = 0);
		virtual ~CommentRequest();

		virtual const QString action() const {
			return "queryArticleListV2";
		}

		virtual void setEntry(const opds::Entry &);
		virtual bool hasNext();
		
		int getBarId();	
		int getMemberStatus();
	
	protected:
		virtual netview::Parser *createParser();
		virtual void responsed(QNetworkReply *, netview::Parser *);
	
	private:
		int barId;
		int memberStatus;
	};
	
	//MoreCommentRequest
	class MoreCommentRequest : public CommentRequest {
	public:
		MoreCommentRequest(QNetworkAccessManager *m, QObject *parent = 0);
		virtual ~MoreCommentRequest();

		virtual const QString action() const {
			return "queryArticleListV2";
		}

		void setEntry(const opds::Entry &);
	
		virtual bool hasNext();
		virtual void next();
		virtual void reset();

	protected:
		virtual void responsed(QNetworkReply *, netview::Parser *);
	
	private:
		void setLastModifiDate(const QString &);

	private:
		bool hasMore;
		QString barId;
		QString objectId;
		QString lastModifiDate;
	};

	//AddCommentRequest
	class AddCommentRequest : public netview::Request {
	public:
		AddCommentRequest(QNetworkAccessManager *m, QObject *parent = 0);
		virtual ~AddCommentRequest();
		
		virtual const QString action() const {
			return "publishArticle";
		}
		void setLink(const opds::Link &);
		int getResultCode();	
		const QString& getResultMsg();

	protected:
		virtual void fragmentResponsed(netview::Parser *) {}
		virtual void fill(QNetworkRequest *);
		virtual int generateResultCode(QNetworkReply *) {
			return 0;
		}
		virtual void responsed(QNetworkReply *, netview::Parser *);
		virtual netview::Parser *createParser();
		void setResultCode(const int &);
		void setResultMsg(const QString &);

	private:
		int code;
		QString message;
	};	

	//AddBarRequest
	class AddBarRequest : public AddCommentRequest {
	public:
		AddBarRequest(QNetworkAccessManager *m, QObject *parent = 0);
		virtual ~AddBarRequest();

		virtual const QString action() const;
		void setBarId(const int &id);
	
		enum {
			SUCCESS = 0,
			HAVE_JOINED = 25015,
			NO_MEMBER = 4,
			MEMBER = 1
		};

	protected:
		virtual void responsed(QNetworkReply *, netview::Parser *);
		virtual netview::Parser *createParser();
		virtual void fill(QNetworkRequest *);
	
	private:
		int code;
	};
	
	//class CommentInfoRequest
	class CommentInfoRequest : public AddCommentRequest {
	public:
		CommentInfoRequest(QNetworkAccessManager *mgr, QObject *parent = 0);
		virtual ~CommentInfoRequest();

		virtual const QString action();

		void setEntry(const opds::Entry &);
		const opds::Entry getEntry();

	protected:	
		virtual void fill(QNetworkRequest *);
		virtual void responsed(QNetworkReply *, netview::Parser *);
		virtual netview::Parser *createParser();
	
	private:
		opds::Entry entry;
	};

	//class QueryBarListRequest
	class QueryBarListRequest : public dangdangv5::FeedRequest {
		Q_OBJECT
	public:
		QueryBarListRequest(QNetworkAccessManager *mgr, QObject *parent = 0);
		virtual ~QueryBarListRequest();
		
		virtual const QString action() const;
		void setBarName(const QString &);
		bool isExist();

	protected:
		virtual void responsed(QNetworkReply *, netview::Parser *);
		virtual netview::Parser *createParser();

	private:
		bool exist;
	};

	//class GetRecommendTagsRequest
	class GetRecommendTagsRequest : public dangdangv5::FeedRequest {
		Q_OBJECT
	public:
		GetRecommendTagsRequest(QNetworkAccessManager *mgr, QObject *parent = 0);
		virtual ~GetRecommendTagsRequest();
	
		virtual const QString action() const;

	protected:
		virtual void responsed(QNetworkReply *, netview::Parser *);
		virtual netview::Parser *createParser();
	};

	//class CreateBarRequest
	class CreateBarRequest : public dangdangv5::FeedRequest {
		Q_OBJECT
	public:
		CreateBarRequest(QNetworkAccessManager *mgr, QObject *parent = 0);
		virtual ~CreateBarRequest();
	
		virtual const QString action() const;
		virtual void setLink(const opds::Link &);
		
		const QString getErrorString();
		int getStatusCode();

	protected:
		virtual void responsed(QNetworkReply *, netview::Parser *);
		virtual netview::Parser *createParser();

	private:
		int code;
		QString error;
	};

}


#endif

