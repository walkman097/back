#ifndef CLOUDFEEDVIEWSHELF_H
#define CLOUDFEEDVIEWSHELF_H

#include "feedview.h"
#include "cloudwidget.h"
#include "localfilefeedrequest.h"

namespace opds {
	class FeedViewDelegate;
}

namespace dangdang {
	
	//class CloudShelfWidget
	class CloudFeedView;
	class CloudFeedViewShelf;
	class CloudShelfWidgetPrivate;
	class CloudShelfWidget : public dangdang::BasicShelfWidget {
		Q_OBJECT
	public:
		CloudShelfWidget(QWidget *parent = 0);	
		~CloudShelfWidget();

		virtual QWidget *getOperateWidget();

		void load();
		void logout();
		bool doOperate(QKeyEvent *e);
	
	private slots:
		void selectedBut(OperationButtonType);
		void dataLoaded();

	private:
		CloudShelfWidgetPrivate *d;
	};
	

	//CloudFeedView 
	class CloudFeedView : public opds::FeedView {
		Q_OBJECT
	public:
		CloudFeedView(opds::FeedRequest *request, QNetworkAccessManager *mgr, QWidget *parent = 0);
		virtual ~CloudFeedView();

		virtual void load();
		virtual void logout();
		virtual bool isEmpty();

	protected:
		virtual opds::FeedRequest* createFeedRequest();

	private slots:
		void dataLoaded();

	private:
		opds::FeedRequest *request;
	};
	
	class CloudEmptyShelf  : public QWidget {
		Q_OBJECT
	public:
		CloudEmptyShelf(QWidget *parent = 0);
		~CloudEmptyShelf();

	private slots:
		void login();

	private:
		eink::MonoTextButton *loginButton;
		QLabel *infoLabel;
	};

	//class CloudFeedViewShelf
	class CloudFeedViewShelf : public eink::BaseWidget {
		Q_OBJECT
	public:
		CloudFeedViewShelf(opds::FeedRequest *request, QNetworkAccessManager *mgr, 
				QWidget *parent = 0);
		virtual ~CloudFeedViewShelf();

		virtual void load();
		virtual void logout();
		virtual bool isEmpty();
	
	signals:
		void openEntry(const opds::Entry &);
		void dataLoaded();

	private:
		QStackedWidget *stack;	
		CloudEmptyShelf *emptyShelf;
		CloudFeedView *feedView;
		opds::FeedViewDelegate *delegate;
	};
	

};

#endif

