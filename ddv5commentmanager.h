#ifndef DDV5COMMENTMANAGER_H
#define DDV5COMMENTMANAGER_H

#include <QString>
#include <QObject>
#include <QWidget>
#include "commentmanager.h"
#include "ddv5rootview.h"
#include "opds.h"

class QGridLayout;

namespace opds {
	class FeedRequest;
	class EntryThumbnailLabel;
}

namespace dangdang {
	class ContentLabel;
}

namespace eink {
	class ScrollArea;
}

class QLabel;
namespace dangdangv5 {
	class CommentRequest;
	class CommentInfoRequest;
	class AddCommentForInfoRequest;
	class AddCommentRequest;
	class AddBarRequest;
	class GetRecommendTagsRequest;
	class QueryBarListRequest;
	class CreateBarRequest;
	class EinkButton;
	class CommentManager : public QObject, public opds::CommentManager {
		Q_OBJECT
	public:
		static CommentManager *getInstance();
		virtual ~CommentManager();
		
		virtual void setEntry(const opds::Entry &);
		virtual opds::FeedRequest *getCommentRequest();
		virtual opds::FeedRequest *getMoreCommentRequest();
		virtual opds::FeedRequest *getQueryBarRequest();
		virtual opds::FeedRequest *getCreatebarRequest();
		virtual void addComment(const opds::Link &);
	
	private:
		CommentManager();

	private:
		static CommentManager *instance;
		CommentRequest *request;
	};
	
	//class AddCommentTask
	class AddCommentTask : public QObject {
		Q_OBJECT
	public:
		AddCommentTask(QObject *parent = 0);
		virtual ~AddCommentTask();
	
		void addComment();

	signals:
		void addCommentFinished();

	private slots:
		void addSlot();
		void addBarSlot();

	private:
		bool inputContent();

	private:
		QString content;
		AddCommentRequest  *addRequest;
		AddBarRequest	*addBarRequest;
	};


	//class CommentEntryView
	class CommentEntryView : public QWidget {
		Q_OBJECT
	public:
		CommentEntryView(QWidget *parent = 0);
		virtual ~CommentEntryView();

		void setEntry(const opds::Entry &);
		void setDetailEntry(const opds::Entry &);
	
	private slots:
		void finishedSlot();

	private:
		void reset();
		void changeFont(QLabel *, int );
		void needShow();
		void setDefaultPixmap(const opds::Entry &);
		QWidget* getWidget();

	private:
		CommentInfoRequest *request;
		QString pixPath;
		eink::ScrollArea *scrollArea;
		opds::EntryThumbnailLabel *thumbnailLabel;
		dangdang::ContentLabel *nameLabel;
		dangdang::ContentLabel *dateLabel;
		dangdang::ContentLabel *contentLabel;
		dangdang::ContentLabel *titleLabel;
	};
	
	//class RecommendTagsView 
	class RecommendTagsView : public QWidget {
		Q_OBJECT
	public:
		RecommendTagsView(QWidget *parent = 0);
		virtual ~RecommendTagsView();
	
	signals:
		void selected(const opds::Entry &); 

	protected:
		void showEvent(QShowEvent *);
		
	private slots:
		void finishedSlot();
		void selectTagSlot();

	private:
		void drawTag();

	private:
		QLabel *contentLab;
		QString loading;
		QString tips;
		QGridLayout *tagLayout;
		QList<EinkButton *> buttonList;
		GetRecommendTagsRequest *request;
	};
	
}

#endif


