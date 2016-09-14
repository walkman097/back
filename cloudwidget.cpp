#include "cloudwidget.h"
#include "cloudshelf.h"
#include "cloudmanager.h"
#include "loginmanager.h"
#include "bookstoreutil.h"
#include "bookstorefactory.h"
#include "button.h"
#include "label.h"
#include "systemmanager.h"
#include "painterutil.h"
#include "zlsapplication.h"
#include <QtGui>
#include <assert.h>

namespace dangdang {
	CloudWidget::CloudWidget(QWidget *parent)
		:dangdang::BasicShelfWidget(parent)
	{
		initWidget();
	}

	CloudWidget::CloudWidget(int row, int column, QWidget *parent)
		:dangdang::BasicShelfWidget(parent)
	{
		initWidget(row, column);
	}

	CloudWidget::CloudWidget(int row, int column, int margin, QWidget *parent)
		:dangdang::BasicShelfWidget(parent)
	{
		initWidget(row, column, margin);
	}
	
	void CloudWidget::initWidget(int row, int column, int margin)
	{
		QStringList qms;
		qms << "libdangdangshelf";
		ZLSApplication::loadTranslations(qms);
		
		buyHideShelf = NULL;
		QVBoxLayout *layout = new QVBoxLayout(this);
		layout->setMargin(margin);
		layout->setSpacing(margin);
		stack = new QStackedWidget(this);
		layout->addWidget(stack);
		cloudManager = opds::BookStoreUtil::getInstance()->factory()->getCloudManager();
		
		assert(cloudManager != NULL);
		
		opds::LocalFileFeedRequest *buyHideRequest = cloudManager->getBuyHideRequest();
		if (buyHideRequest != NULL) {
			buyHideShelf = new CloudShelf(row, column, buyHideRequest, this);
		}
		buyShelf = new CloudShelf(row, column, cloudManager->getBuyRequest(), this);
		borrowShelf = new CloudShelf(row, column, cloudManager->getBorrowRequest(), this);
		vipShelf = new CloudShelf(row, column, cloudManager->getVIPRequest(), this);
		
		emptyWidget = new CloudEmptyWidget(this);
		connect(emptyWidget, SIGNAL(toBookStore()), SIGNAL(toBookStore()));
		
		if (buyHideShelf) {
			shelfList.append(buyHideShelf);
		}
		shelfList.append(buyShelf);
		shelfList.append(borrowShelf);
		shelfList.append(vipShelf);

		for (int i = 0; i<shelfList.count(); ++i) {
			stack->addWidget(shelfList[i]);
			connect(shelfList[i], SIGNAL(selected(const opds::Entry &)),
				SIGNAL(selected(const opds::Entry &)));
			connect(shelfList[i], SIGNAL(dataChanged()), SLOT(dataChanged()));
			connect(shelfList[i], SIGNAL(pageChanged(int,int)), SIGNAL(pageChanged(int,int)));
		}

		stack->addWidget(emptyWidget);
		
		operateWidget = new CloudOperateWidget(this);
		connect(operateWidget, SIGNAL(selectedBut(OperationButtonType)), SLOT(selectedBut(OperationButtonType)));
		//selectedBut(operateWidget->butType());
		stack->setCurrentWidget(buyShelf);
		
		operateWidget->checkHidden();
	}

	CloudWidget::~CloudWidget()
	{
	}

	bool CloudWidget::doOperate(QKeyEvent *e)
	{
		if (e->type() != QEvent::KeyRelease)
			return false;
		switch (e->key()) {
		case Qt::Key_PageDown:
		{
			CloudShelf *shelf = qobject_cast<CloudShelf*>(stack->currentWidget());
			if (shelf)
				shelf->pageDown();
			return true;
		}
		case Qt::Key_PageUp:
		{
			CloudShelf *shelf = qobject_cast<CloudShelf*>(stack->currentWidget());
			if (shelf)
				shelf->pageUp();
			return true;
		}
		default:
			break;
		}
		return false;
	}

	QWidget *CloudWidget::getOperateWidget()
	{
		return operateWidget;
	}

	void CloudWidget::load()
	{
		cloudManager->load();
		if (opds::BookStoreUtil::getInstance()->factory()->getLoginManager()->isLogined()) {
			buyShelf->load();
			vipShelf->load();
			if (buyHideShelf) {
				buyHideShelf->load();
			}
			borrowShelf->load();
			operateWidget->reset();
		} else {
			selectedBut(BUYBUT);//operateWidget->butType());
		}
	}
	
	void CloudWidget::logout()
	{
		buyShelf->logout();
		if (buyHideShelf) {
			buyHideShelf->logout();
		}
		borrowShelf->logout();
		vipShelf->logout();	
		cloudManager->logout();
		
		selectedBut(BUYBUT);
		operateWidget->checkHidden();
	}

	void CloudWidget::dataChanged()
	{
		CloudShelf *shelf = qobject_cast<CloudShelf *>(sender());
		if (!shelf) {
			qWarning() << "CloudWidget" << __func__ << "sender not CloudWidget";
			return;
		}
		if (!opds::BookStoreUtil::getInstance()->factory()->getLoginManager()->isLogined())
			return;
		if ((shelf->isEmpty() && stack->currentWidget() != emptyWidget) ||
		    (!shelf->isEmpty() && stack->currentWidget() == emptyWidget)) {
			qDebug() << "CloudWidget" << __func__ << "change the emptyWidget state";
			selectedBut(operateWidget->butType());
		}
		operateWidget->checkHidden();
	}

	void CloudWidget::selectedBut(OperationButtonType t)
	{
		bool logined = opds::BookStoreUtil::getInstance()->factory()->getLoginManager()->isLogined();
		int current = 0;
		int total = 0;
		if (t == BUYBUT) {
			if (buyShelf->isEmpty() && logined) {
				emptyWidget->setText(tr("To Buy"), tr("No buyed book, you can"));
				emptyWidget->setShowToStore(true);
				stack->setCurrentWidget(emptyWidget);
			} else {
				current = buyShelf->getCurrentPage();
				total = buyShelf->getTotalPage();;
				stack->setCurrentWidget(buyShelf);
			}
		} else if (t == BORROWBUT) {
			if (borrowShelf->isEmpty() && logined) {
				emptyWidget->setText(tr("To Borrow"), tr("No borrowed book, you can"));
				emptyWidget->setShowToStore(true);
				stack->setCurrentWidget(emptyWidget);
			} else {
				current = borrowShelf->getCurrentPage();
				total = borrowShelf->getTotalPage();;				
				stack->setCurrentWidget(borrowShelf);
			}
		} else if (t == BUYHIDEBUT) {
			if (buyHideShelf) {
				if (buyHideShelf->isEmpty() && logined) {
					emptyWidget->setText(tr(""), tr("No buyhide book"));
					emptyWidget->setShowToStore(false);
					stack->setCurrentWidget(emptyWidget);
				} else {
					current = buyHideShelf->getCurrentPage();
					total = buyHideShelf->getTotalPage();;
					stack->setCurrentWidget(buyHideShelf);
				}
			}
		} else if (t == VIPBUTTON) {
			if (vipShelf) {
				if (vipShelf->isEmpty() && logined) {
					emptyWidget->setText(tr(""), tr("No VIP book"));
					emptyWidget->setShowToStore(false);
					stack->setCurrentWidget(emptyWidget);
				} else {
					current = vipShelf->getCurrentPage();
					total = vipShelf->getTotalPage();;					
					stack->setCurrentWidget(vipShelf);
				}
			}
		}
		if (!logined)
			emit pageChanged(0, 0);
		else
			emit pageChanged(current, total);			
	}

	void CloudWidget::setPaintTitle(bool b)
	{
		for (int i = 0; i<shelfList.count(); ++i) {
			shelfList[i]->setFont(font());
			shelfList[i]->setPaintTitle(b);
		}
	}

	void CloudWidget::setShowPageNumber(bool b)
	{
		for (int i = 0; i<shelfList.count(); ++i) {
			shelfList[i]->setShowPageNumber(b);
		}
	}

	//class CloudOperateWidget
	CloudOperateWidget::CloudOperateWidget(QWidget *parent)
		:QWidget(parent)
	{
		font = QApplication::font();
		font.setPointSize(font.pointSize() - 4);
		setFont(font);
		
		QHBoxLayout *layout = new QHBoxLayout(this);
		layout->setMargin(0);
		layout->setSpacing(0);


		buyButton = new eink::MonoTextButton(tr("Buyed"), this);
		buyButton->setHasBorder(false);
		borrowButton = new eink::MonoTextButton(tr("Borrowed"), this);
		borrowButton->setHasBorder(false);
		vipButton = new eink::MonoTextButton(tr("VIP"), this);
		vipButton->setHasBorder(false);

		layout->addWidget(buyButton);
		layout->addWidget(createLineLabel());
		if (opds::BookStoreUtil::getInstance()->factory()->getCloudManager()->getBuyHideRequest()) {
			buyHideWidget = new QWidget(this);
			QHBoxLayout *buyHideLayout = new QHBoxLayout(buyHideWidget);
			buyHideLayout->setSpacing(0);
			buyHideLayout->setMargin(0);
			
			buyHideButton = new eink::MonoTextButton(tr("Hidden"), this);
			buyHideButton->setHasBorder(false);

			buyHideLayout->addWidget(buyHideButton);
			buyHideLayout->addWidget(createLineLabel());
			//QFont f(font);
			//f.setPointSize(font.pointSize() - 2);
			//buyHideButton->setFont(font);
			layout->addWidget(buyHideWidget);
			//buyHideWidget->hide();
						
			//layout->setAlignment(buyHideButton, Qt::AlignHCenter | Qt::AlignBottom);
			connect(buyHideButton, SIGNAL(clicked()), SLOT(clicked()));
			
		} else {
			buyHideButton = NULL;
			buyHideWidget = NULL;
		}		
		layout->addWidget(borrowButton);
		layout->addWidget(createLineLabel());
		layout->addWidget(vipButton);

		connect(buyButton, SIGNAL(clicked()), SLOT(clicked()));
		connect(borrowButton, SIGNAL(clicked()), SLOT(clicked()));
		connect(vipButton, SIGNAL(clicked()), SLOT(clicked()));

		drawButton(BUYBUT);
	}

	CloudOperateWidget::~CloudOperateWidget()
	{
	}

	eink::LineLabel *CloudOperateWidget::createLineLabel()
	{
		const int border = eink::SystemManager::getInstance()->buttonBorderWidth();
		eink::LineLabel *label = new eink::LineLabel(border, Qt::Vertical, this);
		QFontMetrics fm(font);
		label->setFixedHeight(fm.height());
		return label;
	}

	void CloudOperateWidget::reset()
	{
		selected(BUYBUT);
	}

	void CloudOperateWidget::selected(OperationButtonType type)
	{
		drawButton(type);
		emit selectedBut(type);		
	}
	
	void CloudOperateWidget::checkHidden()
	{
		if (!buyHideButton)
			return;
		
		if (!hasHideBooks()) {
			buyHideWidget->hide();
			type = BUYBUT;
		} else {
			QFont f = font;//buyHideButton->font();
			f.setBold(false);
			buyHideWidget->setFont(f);
			buyHideWidget->show();
		}
	}
	
	void CloudOperateWidget::showHidden(bool b)
	{
		if (b) {
			if (buyHideWidget->isHidden())
				buyHideWidget->show();
		} else {
			if (!buyHideWidget->isHidden())
				buyHideWidget->hide();
		}
	}

	bool CloudOperateWidget::hasHideBooks()
	{
		opds::LocalFileFeedRequest * request =
			opds::BookStoreUtil::getInstance()->factory()->getCloudManager()->getBuyHideRequest();
		if (!request)
			return false;
		return !request->getFeed().entryList.empty();
	}
	
	void CloudOperateWidget::clicked()
	{
		QObject* t_sender = sender();
		if (t_sender == buyHideButton) {
			type = BUYHIDEBUT;
		} else if (t_sender == buyButton) {
			type = BUYBUT;
		} else if (t_sender == borrowButton) {
			type = BORROWBUT;
		} else if (t_sender == vipButton) {
			type = VIPBUTTON;
		}
		selected(type);
	}

	OperationButtonType CloudOperateWidget::butType()
	{
		return type;
	}

	void CloudOperateWidget::drawButton(OperationButtonType t)
	{
		type = t;
		QFont boldFont = font;
		boldFont.setBold(true);
		
		if (type == BUYBUT) {
			buyButton->setFont(boldFont);
			borrowButton->setFont(font);
			vipButton->setFont(font);
			if (buyHideButton) 
				buyHideButton->setFont(font);
			checkHidden();			
		} else if (type == BORROWBUT) {
			buyButton->setFont(font);
			vipButton->setFont(font);
			borrowButton->setFont(boldFont);
			if (buyHideButton) 
				buyHideButton->setFont(font);
#if 0
			if (buyHideWidget && !buyHideWidget->isHidden())
				buyHideWidget->hide();
#endif
		} else if (type == BUYHIDEBUT) {
			buyButton->setFont(font);
			vipButton->setFont(font);
			borrowButton->setFont(font);
			if (buyHideButton) 
				buyHideButton->setFont(boldFont);
		} else if (type == VIPBUTTON) {
			vipButton->setFont(boldFont);
			borrowButton->setFont(font);
			buyButton->setFont(font);
			if (buyHideButton) 
				buyHideButton->setFont(font);
#if 0
			if (buyHideWidget && !buyHideWidget->isHidden())
				buyHideWidget->hide();
#endif
		}
	}

	//CloudEmptyWidget
	CloudEmptyWidget::CloudEmptyWidget(QWidget *parent)
		:QWidget(parent)
	{
		QVBoxLayout *layout = new QVBoxLayout(this);

		infoLabel = new QLabel(this);
		infoLabel->setAlignment(Qt::AlignCenter);

		QHBoxLayout *butLayout = new QHBoxLayout;
		bookstoreButton = new eink::MonoTextButton(tr("Bookstore"), this);
		bookstoreButton->setFitTextWidth(false);
		butLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Preferred));
		butLayout->addWidget(bookstoreButton);
		butLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Preferred));

		layout->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Expanding));
		layout->addWidget(infoLabel);
		layout->addLayout(butLayout);
		layout->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Expanding));

		connect(bookstoreButton, SIGNAL(clicked()), SIGNAL(toBookStore()));
	}

	CloudEmptyWidget::~CloudEmptyWidget()
	{
	}

	void CloudEmptyWidget::setText(const QString &title, const QString &msg)
	{
		bookstoreButton->setText(title);
		infoLabel->setText(msg);
	}

	void CloudEmptyWidget::setShowToStore(bool b)
	{
		if (b) {
			bookstoreButton->show();
		} else {
			bookstoreButton->hide();
		}
	}
}
