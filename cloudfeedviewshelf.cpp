#include "cloudfeedviewshelf.h"
#include "feedviewdelegate.h"
#include "bookstoreutil.h"
#include "loginmanager.h"
#include "cloudmanager.h"
#include "bookstorefactory.h"
#include "zlsapplication.h"

#include <QDebug>
#include <QtGui>

namespace dangdang {


	//class CloudShelfWidget
	class CloudShelfWidgetPrivate {
	public:	
		CloudShelfWidgetPrivate() {
		};

		CloudFeedViewShelf *buyHideShelf;
		CloudFeedViewShelf *buyShelf;
		CloudFeedViewShelf *borrowShelf;
		CloudFeedViewShelf *vipShelf;
		OperationButtonType type;
		CloudEmptyWidget *emptyWidget;
		CloudOperateWidget *operateWidget;
		opds::CloudManager *cloudManager;
		QStackedWidget *stack;
		QList<CloudFeedViewShelf *> shelfList;
	};
	
	CloudShelfWidget::CloudShelfWidget(QWidget *parent)
		:dangdang::BasicShelfWidget(parent)
		,d(new CloudShelfWidgetPrivate)
	{
		QVBoxLayout *layout = new QVBoxLayout(this);	
		d->stack = new QStackedWidget(this);

		d->cloudManager = opds::BookStoreUtil::getInstance()->factory()->getCloudManager();
		if (d->cloudManager == NULL) {
			qDebug() << "CloudShelfWidget" << __func__ << "WARNING::cloudmanager is null !!!!!";
		}
		
		QNetworkAccessManager *mgr = opds::BookStoreUtil::getInstance()->getNetworkMgr();
		d->buyHideShelf = new CloudFeedViewShelf(d->cloudManager->getBuyHideRequest(), mgr); 
		d->buyShelf = new CloudFeedViewShelf(d->cloudManager->getBuyRequest(), mgr); 
		d->borrowShelf = new CloudFeedViewShelf(d->cloudManager->getBorrowRequest(), mgr);
		d->vipShelf = new CloudFeedViewShelf(d->cloudManager->getVIPRequest(), mgr);
		
		d->shelfList.append(d->buyHideShelf);
		d->shelfList.append(d->buyShelf);
		d->shelfList.append(d->borrowShelf);
		d->shelfList.append(d->vipShelf);
		
		for (int i = 0; i < d->shelfList.count(); ++i) {
			d->stack->addWidget(d->shelfList[i]);
			connect(d->shelfList[i], SIGNAL(openEntry(const opds::Entry &)), 
					SLOT(selected(const opds::Entry &)));
			connect(d->shelfList[i], SIGNAL(dataLoaded()), SLOT(dataLoaded()));
		}
		
		d->emptyWidget = new CloudEmptyWidget(this);
		connect(d->emptyWidget, SIGNAL(toBookStore()), SIGNAL(toBookStore()));
		d->stack->addWidget(d->emptyWidget);
		
		d->operateWidget = new CloudOperateWidget(this);
		connect(d->operateWidget, SIGNAL(selectedBut(OperationButtonType)), 
				SLOT(selectedBut(OperationButtonType)));
		d->operateWidget->checkHidden();
		d->stack->setCurrentWidget(d->buyShelf);
		
		layout->addWidget(d->operateWidget);
		layout->addWidget(d->stack);
	}

	CloudShelfWidget::~CloudShelfWidget()
	{
		delete d;
		d = NULL;
	}

	QWidget* CloudShelfWidget::getOperateWidget()
	{
		return d->operateWidget;
	}

	void CloudShelfWidget::load()
	{
		d->type = BUYBUT;
		if (opds::BookStoreUtil::getInstance()->factory()->getLoginManager()->isLogined()) {
			d->buyShelf->load();
			d->operateWidget->reset();
		} else {
			d->buyShelf->logout();
			selectedBut(BUYBUT);
		}
	}

	void CloudShelfWidget::logout()
	{
		d->buyShelf->logout();
		d->buyHideShelf->logout();
		d->borrowShelf->logout();
		d->vipShelf->logout();	
		d->cloudManager->logout();
		
		selectedBut(BUYBUT);
		d->operateWidget->checkHidden();
	}

	bool CloudShelfWidget::doOperate(QKeyEvent *e)
	{
		if (e->type() != QEvent::KeyRelease)
			return false;
		switch (e->key()) {
		case Qt::Key_PageDown:
		case Qt::Key_PageUp:
		{
			//TODO 
			return false;
		}
		default:
			break;
		}
		return false;
	}

	void CloudShelfWidget::selectedBut(OperationButtonType type)
	{
		if (d->type == type) 
			return;

		bool logined = opds::BookStoreUtil::getInstance()->factory()->getLoginManager()->isLogined();

		d->type = type;
		switch (type) {
		case BUYBUT:
		{
			if (logined) {
				d->buyShelf->load();	
				d->stack->setCurrentWidget(d->buyShelf);
			} else {
				d->buyShelf->logout();
			}
			break;
		}
		case BUYHIDEBUT:
		{
			if (logined) {
				d->buyHideShelf->load();	
				d->stack->setCurrentWidget(d->buyHideShelf);
			} else {
				d->buyHideShelf->logout();
			}
			break;
		}
		case BORROWBUT:
		{
			if (logined) {
				d->borrowShelf->load();
				d->stack->setCurrentWidget(d->borrowShelf);
			} else {
				d->borrowShelf->logout();
			}
			break;
		}
		case VIPBUTTON:
		{
			if (logined) {
				d->vipShelf->load();
				d->stack->setCurrentWidget(d->vipShelf);
			} else {
				d->vipShelf->logout();
			}
			break;
		}
		default:
			break;
		}
	}

	void CloudShelfWidget::dataLoaded()
	{
		if (!opds::BookStoreUtil::getInstance()->factory()->getLoginManager()->isLogined())
			return;
		d->operateWidget->checkHidden();
		
		CloudFeedViewShelf *view = dynamic_cast<CloudFeedViewShelf *>(sender());
		if (view->isEmpty()) {
			switch (d->type) {
			case BUYBUT:
			{
				d->emptyWidget->setText(tr("To Buy"), tr("No buyed book, you can"));
				d->emptyWidget->setShowToStore(true);
				d->stack->setCurrentWidget(d->emptyWidget);
				break;
			}
			case BUYHIDEBUT:
			{
				d->emptyWidget->setText(tr(""), tr("No buyhide book"));
				d->emptyWidget->setShowToStore(false);
				d->stack->setCurrentWidget(d->emptyWidget);
				break;
			}
			case BORROWBUT:
			{
				d->emptyWidget->setText(tr("To Borrow"), tr("No borrowed book, you can"));
				d->emptyWidget->setShowToStore(true);
				d->stack->setCurrentWidget(d->emptyWidget);
				break;
			}
			case VIPBUTTON:
			{
				d->emptyWidget->setText(tr("To VIP channel"), tr("No VIP book, you can"));
				d->emptyWidget->setShowToStore(true);
				d->stack->setCurrentWidget(d->emptyWidget);
				break;
			}
			default:
				break;
			}
		} else {
			d->stack->setCurrentWidget(view);
		}
	}
	
	//class CloudFeedView
	CloudFeedView::CloudFeedView(opds::FeedRequest *request, QNetworkAccessManager *mgr, QWidget *parent)
		:opds::FeedView(mgr, parent)
	{
		this->request = request;
		connect(this, SIGNAL(dataLoaded()), this, SLOT(dataLoaded()));
	}

	CloudFeedView::~CloudFeedView()
	{
	}

	void CloudFeedView::load()
	{
		getFeedRequest()->reset();
		getFeedRequest()->clearFeed();
		loadData();
	}
	
	opds::FeedRequest *CloudFeedView::createFeedRequest()
	{
		return request;
	}

	void CloudFeedView::dataLoaded()	
	{
		if (!getFeedRequest()->hasNext())
			fillReserved();
	}
	
	void CloudFeedView::logout()
	{
		getFeedRequest()->clearFeed();
		getModel()->setCurrentPage(0); 
	}

	bool CloudFeedView::isEmpty()
	{
		return (getFeedRequest()->getFeed().entryList.size() == 0);
	}

	//class CloudEmptyShelf
	CloudEmptyShelf::CloudEmptyShelf(QWidget *parent)
		:QWidget(parent)
	{
		QVBoxLayout *layout = new QVBoxLayout(this);
		infoLabel = new QLabel(this);
		infoLabel->setText(tr("You not login,please login"));
		infoLabel->setAlignment(Qt::AlignCenter);

		QHBoxLayout *butLayout = new QHBoxLayout;
		loginButton = new eink::MonoTextButton(tr("Login"), this);
		loginButton->setFitTextWidth(false);
		butLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Preferred));
		butLayout->addWidget(loginButton);
		butLayout->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Preferred));

		layout->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Expanding));
		layout->addWidget(infoLabel);
		layout->addLayout(butLayout);
		layout->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Expanding));

		connect(loginButton, SIGNAL(clicked()), SIGNAL(login()));
	}

	CloudEmptyShelf::~CloudEmptyShelf()
	{
	}

	void CloudEmptyShelf::login()
	{
		ZLSApplication::execute("loginuserinfo");
	}
	
	//class CloudFeedViewShelf
	CloudFeedViewShelf::CloudFeedViewShelf(opds::FeedRequest *request, QNetworkAccessManager 
			*mgr, QWidget *parent)
		:eink::BaseWidget(parent)
	{
		QVBoxLayout *layout = new QVBoxLayout(this);
		
		stack = new QStackedWidget(this);
		emptyShelf = new CloudEmptyShelf;
		feedView = new CloudFeedView(request, mgr);

		stack->addWidget(emptyShelf);
		stack->addWidget(feedView);
		stack->setCurrentWidget(feedView);
		layout->addWidget(stack);
		
		bool show = opds::BookStoreUtil::getInstance()->isInfoShow();
		feedView->setItemHeight(opds::BookStoreUtil::getInstance()->getBookPixmap()->height());
		feedView->setHasPageMgr(false);
		feedView->setInfoLabelVisible(show);
		feedView->setShowInfo(false);
		delegate = new opds::FeedViewDelegate(this);
		feedView->setDelegate(delegate);
		feedView->setLayoutSpacing(1);
		feedView->setLayoutMargin(1);

		connect(feedView, SIGNAL(openEntry(const opds::Entry &)), SIGNAL(openEntry(const opds::Entry &)));
		connect(feedView, SIGNAL(dataLoaded()), SIGNAL(dataLoaded()));
	}

	CloudFeedViewShelf::~CloudFeedViewShelf()
	{
		delete delegate;
		delegate = NULL;
	}

	void CloudFeedViewShelf::load()
	{
		feedView->load();
		stack->setCurrentWidget(feedView);
	}

	void CloudFeedViewShelf::logout()
	{
		feedView->logout();
		stack->setCurrentWidget(emptyShelf);
	}

	bool CloudFeedViewShelf::isEmpty()
	{
		return feedView->isEmpty();
	}


};


