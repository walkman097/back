#ifndef CLOUDWIDGET_H
#define CLOUDWIDGET_H

#include "basicshelfwidget.h"
#include "opds.h"
#include "tobserver.h"

namespace eink {
	class MonoTextButton;
	class LineLabel;
}
namespace opds {
	class CloudManager;
	class LocalFileFeedRequest;
}
class QStackedWidget;
class QLabel;
namespace dangdang {
	enum OperationButtonType {
		BUYHIDEBUT,
		BUYBUT,
		BORROWBUT,
		VIPBUTTON,
	};

	class CloudShelf;
	class CloudEmptyWidget;
	class CloudOperateWidget;
	class CloudWidget : public dangdang::BasicShelfWidget {
		Q_OBJECT
	public:
		CloudWidget(int row, int column, int margin, QWidget *parent = 0);	
		CloudWidget(int row, int column, QWidget *parent = 0);
		CloudWidget(QWidget *parent = 0);	
		~CloudWidget();

		virtual QWidget *getOperateWidget();

		void load();
		void logout();

		void setPaintTitle(bool);
		void setShowPageNumber(bool);

		bool doOperate(QKeyEvent *e);

	signals:
		void selected(const opds::Entry &);
		void toBookStore();
		void pageChanged(int, int);

	private slots:
		void selectedBut(OperationButtonType);
		void dataChanged();

	private:
		void initWidget(int row = 2, int column = 3, int margin = 0);

	private:
		CloudShelf* buyHideShelf;
		CloudShelf *buyShelf;
		CloudShelf *borrowShelf;
		CloudShelf *vipShelf;
		CloudEmptyWidget *emptyWidget;
		CloudOperateWidget *operateWidget;
		QStackedWidget *stack;
		opds::CloudManager *cloudManager;
		QList<CloudShelf *> shelfList;
	};

	class CloudOperateWidget : public QWidget {
		Q_OBJECT
	public:
		CloudOperateWidget(QWidget *parent = 0);
		~CloudOperateWidget();

		void reset();

		OperationButtonType butType();

		void checkHidden();
		void showHidden(bool );

	signals:
		void selectedBut(OperationButtonType);

	private slots:
		void clicked();

	private:
		void drawButton(OperationButtonType);
		bool hasHideBooks();
		void selected(OperationButtonType);
		eink::LineLabel *createLineLabel();

	private:
		eink::MonoTextButton* buyHideButton;
		eink::MonoTextButton *buyButton;
		eink::MonoTextButton *borrowButton;
		eink::MonoTextButton *vipButton;
		QWidget *buyHideWidget;
		QFont font;
		OperationButtonType type;
	};

	class CloudEmptyWidget : public QWidget {
		Q_OBJECT
	public:
		CloudEmptyWidget(QWidget *parent = 0);
		~CloudEmptyWidget();

		void setText(const QString &, const QString &);
		void setShowToStore(bool);

	signals:
		void toBookStore();

	private:
		eink::MonoTextButton *bookstoreButton;
		QLabel *infoLabel;
	};
}

#endif
