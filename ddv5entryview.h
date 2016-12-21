#ifndef DDV5ENTRYVIEW_H
#define DDV5ENTRYVIEW_H

#include <QWidget>
#include "opds.h"
#include "tobserver.h"
#include "entryview.h"

class QLabel;
class QStackedLayout;
class QStackedWidget;

namespace eink {
	class ScrollArea;
	class EinkButton;
	class PixmapButton;
	class ButtonManager;
	class PopupMenuButton;
}

namespace opds {
	class FeedView;
	class LoginManager;
	class ThumbnailView;
	class CommentListView;
	class EntryThumbnailLabel;
}

namespace dangdang {
	class HonorLabel;
	class ClickLabel;
	class ContentLabel;
	class EntryCommand;
	class DownloadButton;
	class DDThumbnailLabel;
	class EntryButtonWidget;
}

namespace dangdangv5 {
	class ClickLabel;
	class ContentView;
	class CommentView;
	class CommentView;
	class SummaryView;
	class EntryCommand;	
	class ContentLabel;
	class PictureLabel;
	class ShareManager;
	class CopyRightView;
	class AddCommentTask;
	class CommentRequest;
	class CommentManager;
	class AlsoBuyCommand;
	class BookDetailView;
	class CommentListView;
	class ContentListView;
	class ButtonArrayLabel;
	class CreateBarRequest;
	class FeedViewDelegate;
	class CommentEntryView;
	class RecommendTagsView;
	class EntryButtonWidget;
	class BookCommentCommand;
	class QueryBarListRequest;
	class EntryView : public opds::EntryView, public TObserver {
		Q_OBJECT
	public:
		EntryView(QWidget *parent = 0);
		virtual ~EntryView();

		virtual void reload();
		virtual void clear();
		virtual void update(TSubject *);
		virtual bool doOperate(QKeyEvent *);
		virtual void setEntry(const opds::Entry &);

		static bool checkLogin();
		void setHasBackKey(bool);
		void setAuthorLabelEnabled(bool );
	
	public slots:
		virtual void setDetailEntry(const opds::Entry &);

	private:
		void reset();
		void updateCopyRight();	
		void raiseWidget(QWidget *);
		void tipsMessage(const QString &);
		
	private slots:
		void addComment();
		void openCommentEntry(const opds::Entry &);
		//void selectTagSlot(const opds::Entry &);
		void currentSelectedSlot(const QString &);
		void showDetailContent(const QString &);
		void entryChanged(const opds::Entry &);
		void addCommentFinished();

	private:
		opds::Entry entry;
		QStackedLayout *stack;
		SummaryView *summaryView;
		CopyRightView *copyRightView;
		ButtonArrayLabel *arrayLabel;
		ContentView *contentListView;
		CommentView *commentListView;
		BookDetailView *bookInfoWidget;
		CommentManager *commentManager;
		AddCommentTask *addCommentTask;
		QMap<QString, QWidget *> widgets;
		CommentEntryView *commentEntryView;
		RecommendTagsView *recommendTagsView;
	};
	
	//class BookDetailView
	class BookDetailView : public QWidget, public TObserver {
		Q_OBJECT 
	public:
		BookDetailView(QWidget *parent = 0);
		virtual ~BookDetailView();
		
		virtual void reset();
		virtual void clear();
		virtual bool doOperate(QKeyEvent *);
		virtual void update(TSubject *);
		virtual void setEntry(const opds::Entry &);
		virtual void setDetailEntry(const opds::Entry &);
		virtual void setAuthorLabelEnabled(bool );
	
	signals:
		void openAuthor(const QString &);
		void showDetailContent(const QString &);
		void entryChanged(const opds::Entry &);
		void getDetailEntry(const opds::Entry &);

	private slots:
		void favoriteSlot();
		void cartSlot();
		void shareSlot();
		void delayShowSlot();
		void authorMoreSlot();
		void collapseSlot();    
		void refreshAlsoBuySlot();
		void alsoBuySelected(const opds::Entry &);  
		
	private:
		void back();
		void drawTitle();
		void drawAuthor();
		void changeFont();
		void initCommands(); 
		void drawPublisher();
		void showFixContent();
		void handlerPriceLabel();
		void handleOriginalBook();
		void favoriteFinished(bool );
		void imgDownloaded(const QString &);
		void executeCommands(const opds::Entry &);
		
		const QString getPrice();
		const QString getContent();
		const QString getLastChapter();
		const QStringList getAuthors();
		
		bool hasBuy();
		bool hasStore();
		bool isFreeBook();
		bool isPresetBook();
		bool isOriginBook();
		bool isSupportBook();
		int  getIntPrice(const opds::Entry &);
		
	private:
		QList<dangdangv5::EntryCommand *> requestList;
		QList<opds::Entry> historyEntry;
		AlsoBuyCommand *alsoBuyCommand;
		
		opds::Entry entry;
		opds::EntryThumbnailLabel *thumbnailLabel;
		
		dangdang::HonorLabel *honorLabel;
		dangdang::ContentLabel *priceLabel;
		dangdang::ContentLabel *publishLabel;
		dangdang::ContentLabel *titleLabel;
		dangdang::ContentLabel *chapterLab;
		dangdang::ClickLabel *authorLabel;
		
		PictureLabel *cartLabel;
		PictureLabel *favoriteLabel;
		ContentLabel *contentLabel;
		
		eink::PixmapButton *refreshBut;
		opds::ThumbnailView *alsoBuyView;
		EntryButtonWidget *butWidget;

		QList<QLabel *> labelList;
		QString addFavoriteStr;
		QString removeFavoriteStr;
		QString addCartStr;
		QString addCartFail;
		QString removeCartStr;
		QStackedLayout *stack;
	};

	//class ContentView
	class ContentView : public QWidget {
		Q_OBJECT
	public:
		ContentView(QWidget *parent = 0);
		virtual ~ContentView();
	
		void loadEntry(const opds::Entry &);
	
	private slots:
		void dataLoaded();
	
	private:
		void initFeedView(opds::FeedView *, bool setDelegate = true);

	private:
		QStackedWidget *stack;
		FeedViewDelegate *delegate;
		dangdang::ContentLabel *chapterLabel;
		dangdangv5::ContentListView *contentListView;	
	};
	
	//class CommentView
	class CommentView : public QWidget {
		Q_OBJECT
	public:
		CommentView(QWidget *parent = 0);
		virtual ~CommentView();

		void loadEntry(const opds::Entry &);
		void loadComment(const opds::Entry &);
		void showEmpty();

	signals:
		void addComment();
		void openCommentEntry(const opds::Entry &);
	
	private slots:
		void dataLoaded();
	
	private:
		void initFeedView(opds::FeedView *, bool setDelegate = true);

	private:
		QStackedWidget *stack;
		FeedViewDelegate *delegate;
		dangdang::ContentLabel *commentLabel;
		dangdangv5::BookCommentCommand *command;
		dangdangv5::CommentListView *commentListView;
	};
	
}

#endif


