#ifndef DDV5ENTRYCOMMAND_H
#define DDV5ENTRYCOMMAND_H

#include <QObject> 
#include "opds.h"

namespace netview {
	class Request;
}

namespace opds {
	class Entry;
	class FeedView;
	class FeedRequest;
	class CommentListView;
	class ThumbnailView;
}
namespace dangdangv5 {
	class EntryView;
	class FeedRequest;
	class CommentRequest;
	class BookInfoRequest;
	class AlsoBuyRequest;
	class BookDetailView;
	class EntryCommand : public QObject {
	public:
		virtual ~EntryCommand() {
		}
		
		virtual void execute(const opds::Entry &) = 0;
	};

	class BookInfoCommand : public EntryCommand { 
		Q_OBJECT
	public:
		BookInfoCommand(dangdangv5::BookDetailView *);
		virtual ~BookInfoCommand();

		virtual void execute(const opds::Entry &);

	private slots:
		void finished();

	private:
		dangdangv5::BookDetailView *view;
		dangdangv5::BookInfoRequest *request;
	};

	class BookCommentCommand : public EntryCommand {
		Q_OBJECT
	public:
		BookCommentCommand(QObject *parent = 0);
		virtual ~BookCommentCommand();
		virtual void execute(const opds::Entry &);

	private:
		CommentRequest *request;
	};

	class AlsoBuyCommand : public EntryCommand { 
		Q_OBJECT
	public:
		AlsoBuyCommand(opds::ThumbnailView *);
		virtual ~AlsoBuyCommand();

		virtual void execute(const opds::Entry &);
		virtual void refresh(const opds::Entry &);
	
	private slots:
		void finishedSlot();

	private:
		void setUrl(const opds::Entry &);
		void clear();

	private:
		int start;
		int end;
		int total;
		dangdangv5::AlsoBuyRequest *request;		
	};
	
}

#endif


