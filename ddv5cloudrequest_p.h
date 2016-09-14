#ifndef DDv5CLOUDREQUEST_P_H
#define DDv5CLOUDREQUEST_P_H

#include "ddv5feedrequest.h"
#include "ddv5cloudparser.h"

namespace dangdangv5 {
	
	//class VipBookRequest 
	class VipBookRequest : public dangdangv5::LocalFileFeedRequest {
		Q_OBJECT
	public:
		VipBookRequest(const QString &, QNetworkAccessManager *mgr, QObject *parent = 0);
		virtual ~VipBookRequest();

		virtual const QString action() const;
		virtual void execute();
		virtual bool hasNext();
	
	protected:
		virtual netview::Parser *createParser();
		virtual void responsed(QNetworkReply *, netview::Parser *);
		virtual bool entryMatched(const opds::Entry &, const opds::Entry &);

	private:
		QString saveFilePath;
	};

	//class BorrowRequest
	class BorrowRequest : public dangdangv5::LocalFileFeedRequest {
		Q_OBJECT
	public:
		BorrowRequest(const QString &, QNetworkAccessManager *mgr, QObject *parent = 0);
		virtual ~BorrowRequest();

		virtual const QString action() const;
		virtual void execute();
		virtual bool hasNext();
	
	protected:
		virtual netview::Parser *createParser();
		virtual bool entryMatched(const opds::Entry &, const opds::Entry &);
	
	private:
		QString saveFilePath;
	};
		
	//class BuyRequest
	class BuyRequest : public dangdangv5::LocalFileFeedRequest {
		Q_OBJECT
	public:
		BuyRequest(QNetworkAccessManager *mgr, 
			const QString type = BuyBookParser::NO_HIDE, QObject *parent = 0);
		virtual ~BuyRequest();

		virtual const QString action() const;
		virtual void execute();
		virtual void next();
		virtual void reset();
		virtual bool hasNext();
	
	protected:
		virtual netview::Parser *createParser();
		virtual void responsed(QNetworkReply *, netview::Parser *);
	
	private:
		bool doNext;
		QString type;
		QString lastAuthorityId;
	};

}

#endif
