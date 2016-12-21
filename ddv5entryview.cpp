#include "ddv5entryview.h"
#include "ddv5cloudmanager.h"
#include "ddv5entrycommand.h"
#include "ddv5entryparser.h"
#include "ddv5contentrequest.h"
#include "ddv5favoriterequest.h"
#include "ddv5feedviewdelegate.h"
#include "ddv5stackview.h"
#include "opds/imageutil.h"
#include "opds/constants.h"
#include "opds/thumbnailview.h"
#include "ddv5favoritemanager.h"
#include "ddv5shoppingcartmanager.h"
#include "ddv5commentrequest.h"
#include "ddv5commentmanager.h"
#include "zlsapplication.h"
#include "view/textinputdialog.h"
#include "model/imdef.h"
#include "systemmanager.h"
#include "opds/bookstoreutil.h"
#include "bookstorefactory.h"
#include "stringutil.h"
#include "einkmediawidget.h"
#include "popupmenu.h"
#include "einklocal.h"
#include "opdsutil.h"
#include "label.h"
#include "painterutil.h"
#include "commentlistview.h"
#include "ddv5entrybuttonwidget.h"
#include "entrythumbnaillabel.h"
#include "dangdang/presetbookmanager.h"
#include "qrcodedialog.h"

#include <QtGui>
#include <QStackedLayout>
#include <QSettings>

#define SEPERATOR (": ")
#define CONTENT_LINE  (3)

namespace dangdangv5 {

	EntryView::EntryView(QWidget *parent)
		:opds::EntryView(parent)
	{
		QStringList qms;
		qms << "libdangdangv5";
		ZLSApplication::loadTranslations(qms);
		
		entry.clear();
		eink::EinkLocal::getInstance()->setCodecLocal();
		QVBoxLayout *layout = new QVBoxLayout(this);
		layout->setSpacing(0);
		layout->setMargin(0);
		QStringList array;		
		array << tr("BookInfo") << tr("CopyRight") << tr("Content") << tr("Comment");
		arrayLabel = new ButtonArrayLabel(array, parent, 
				eink::SystemManager::getInstance()->useVirtualBackKey());
		QSettings cfg(
			opds::BookStoreUtil::getInstance()->getDefaultConfig(), QSettings::IniFormat);
		cfg.beginGroup("view");
		int height = cfg.value("head_height", 60).toInt();
		cfg.endGroup();
		arrayLabel->setFixedHeight(height);
		connect(arrayLabel, SIGNAL(currentSelected(const QString &)), SLOT(currentSelectedSlot(const QString &)));

		layout->addWidget(arrayLabel);

		stack = new QStackedLayout;
		stack->setSpacing(0);
		stack->setMargin(0);
		copyRightView = new CopyRightView(this);
		summaryView = new SummaryView(this);
		commentEntryView = new CommentEntryView(this);
		bookInfoWidget = new BookDetailView(this); 
		commentListView = new CommentView(this); 
		contentListView = new ContentView(this);

		stack->addWidget(bookInfoWidget);
		stack->addWidget(copyRightView);
		stack->addWidget(contentListView);
		stack->addWidget(commentListView);
		stack->addWidget(summaryView);
		stack->addWidget(commentEntryView);
		layout->addLayout(stack);
		layout->addStretch();
	
		widgets.insert(array[0], bookInfoWidget);
		widgets.insert(array[1], copyRightView);
		widgets.insert(array[2], contentListView);
		widgets.insert(array[3], commentListView);

		connect(bookInfoWidget, SIGNAL(openAuthor(const QString &)), 
				this, SIGNAL(openAuthor(const QString &)));
		connect(bookInfoWidget, SIGNAL(showDetailContent(const QString &)), 
				this, SLOT(showDetailContent(const QString &)));
		connect(bookInfoWidget, SIGNAL(getDetailEntry(const opds::Entry &)), 
				this, SLOT(setDetailEntry(const opds::Entry &)));
		connect(commentListView, SIGNAL(openCommentEntry(const opds::Entry &)), 
				this, SLOT(openCommentEntry(const opds::Entry &)));
		connect(commentListView, SIGNAL(addComment()), this, SLOT(addComment()));

		commentManager = CommentManager::getInstance();
		recommendTagsView = new RecommendTagsView(this);
		stack->addWidget(recommendTagsView);
		
		addCommentTask = new AddCommentTask;
		connect(addCommentTask, SIGNAL(addCommentFinished()), this, SLOT(addCommentFinished()));
	}

	EntryView::~EntryView()
	{
	}

	void EntryView::reload()
	{
		qDebug() << "EntryView" << __func__ << "wifi connected and begin relod information !!!";
		if (entry.isValid() && !entry.id.isEmpty()) {
			if (stack->currentWidget() == commentListView) {
				commentListView->loadEntry(entry);
			} else if (stack->currentWidget() == contentListView) {
				contentListView->loadEntry(entry);
			} else {
				setEntry(entry);
			}
		}
	}
	
	bool EntryView::doOperate(QKeyEvent *e)
	{
		if (e->type() != QEvent::KeyRelease)
			return false;
		
		if (stack->currentWidget() == summaryView) {
			raiseWidget(bookInfoWidget);
			return true;
		} else if (stack->currentWidget() == commentEntryView) {
			raiseWidget(commentListView);
			return true;
		} else if (stack->currentWidget() == bookInfoWidget) {
			if (bookInfoWidget->doOperate(e))
				return true;
			else 
				return false;
		}
		
		return false;
	}
	
	void EntryView::clear()
	{
		bookInfoWidget->clear();
	}

	void EntryView::reset()
	{
		arrayLabel->reset();
		bookInfoWidget->reset();
		raiseWidget(bookInfoWidget);
	}
	
	void EntryView::update(TSubject *)
	{
		//TODO
	}

	void EntryView::setEntry(const opds::Entry &e)
	{
		if (e.id.isEmpty() || !e.isValid()) {
			qDebug() << "EntryView"	<< __func__ << "entry id isEmpty or is not valid !!!!!";
			return;
		}

		reset();
		entry = e;
		raiseWidget(bookInfoWidget);
		bookInfoWidget->setEntry(e);
	}

	void EntryView::setDetailEntry(const opds::Entry &e)
	{
		if (e.id.isEmpty() || !e.isValid()) {
			qDebug() << "EntryView"	<< __func__ << "entry id isEmpty or is not valid !!!!!";
			return;
		}

		entry = e;
		if (stack->currentWidget() == copyRightView) {
			updateCopyRight();	
		}
	}
	
	void EntryView::entryChanged(const opds::Entry &e)
	{
		qDebug() << "EntryView" << __func__ << "handler entry changed !!!!";
		entry = e;
		arrayLabel->reset();
	}

	bool EntryView::checkLogin()
	{
		opds::LoginManager *loginManager = 
			opds::BookStoreUtil::getInstance()->factory()->getLoginManager();
		if (!loginManager->isLogined()) {
			eink::EinkMessageBox box;
			box.setMessage(tr("You have not login, please login first."));
			box.setTitle(tr("Login"));
			box.setOkButtonText(tr("Login Now"));
			box.exec();
			if (eink::EinkMessageBox::STATE_OK == box.getState()) {
				ZLSApplication::execute("loginuserinfo", "login");
			}
			return false;
		}
		return true;
	}
	
	void EntryView::updateCopyRight()
	{
		copyRightView->loadEntry(entry);
	}
	
	void EntryView::tipsMessage(const QString &message) 
	{
		eink::EinkMessageBox::timeoutInformation(
			this,
			tr("Tips"),
			message,
			3,
			eink::EinkMessageBox::NONE);
	}

	void EntryView::currentSelectedSlot(const QString &selected)
	{
		raiseWidget(widgets[selected]);
		if (stack->currentWidget() == copyRightView) {
			copyRightView->loadEntry(entry);
		} else if (stack->currentWidget() == contentListView) {
			contentListView->loadEntry(entry);
		} else if (stack->currentWidget() == commentListView) {
			commentListView->loadEntry(entry);
		}
	}

	void EntryView::raiseWidget(QWidget *widget)
	{
		stack->setCurrentWidget(widget);
	}

	void EntryView::openCommentEntry(const opds::Entry &entry)
	{
		qDebug() << "EntryView" << __func__ << "commentListView openEntry";
		commentEntryView->setEntry(entry);
		raiseWidget(commentEntryView);
	}
	
	void EntryView::addComment()
	{
		if (!checkLogin()) {
			return;
		}
		
		//TODO
		addCommentTask->addComment();
	}
	
	void EntryView::addCommentFinished()
	{
		qDebug() << "EntryView" << __func__ << "add Comment success finished !!!!!";
		commentListView->loadEntry(entry);
		raiseWidget(commentListView);
	}

	void EntryView::setAuthorLabelEnabled(bool b)
	{
		bookInfoWidget->setAuthorLabelEnabled(b);
	}

	void EntryView::setHasBackKey(bool b)
	{
		arrayLabel->setHasBackKey(b);
	}
	
	void EntryView::showDetailContent(const QString &content)
	{
		summaryView->loadContent(content);
		raiseWidget(summaryView);
	}

	//class BookDetailView
	BookDetailView::BookDetailView(QWidget *parent)
		:QWidget(parent)
	{
		const int margin = eink::SystemManager::getInstance()->getMargin();
		addFavoriteStr = tr("Add favorite");
		removeFavoriteStr = tr("Delete favorite");
		addCartStr = tr("Add cart");
		addCartFail = tr("Shopping cart full");
		removeCartStr = tr("Delete cart");
		
		QVBoxLayout *layout = new QVBoxLayout(this);
		layout->setMargin(margin);
		layout->setSpacing(margin);
		
		QHBoxLayout *baseMsgLayout = new QHBoxLayout;
		thumbnailLabel = new opds::EntryThumbnailLabel(parent);
		thumbnailLabel->setAlignment(Qt::AlignCenter);
	
		QVBoxLayout *titleLayout = new QVBoxLayout;
		titleLayout->setMargin(0);
		titleLayout->setSpacing(0);
	
		QHBoxLayout *authorLayout = new QHBoxLayout;
		authorLabel = new dangdang::ClickLabel(dangdang::NORMAL_FONT, parent);
		authorLabel->setAlignment(Qt::AlignLeft);
		authorLayout->addWidget(authorLabel, 1);
		honorLabel = new dangdang::HonorLabel(this);
		publishLabel = new dangdang::ContentLabel(dangdang::NORMAL_FONT, this);
		priceLabel = new dangdang::ContentLabel(dangdang::NORMAL_FONT, this);
		chapterLab = new dangdang::ContentLabel(dangdang::NORMAL_FONT, this);
		titleLabel = new dangdang::ContentLabel(dangdang::NORMAL_FONT, this);
		titleLabel->setAlignment(Qt::AlignTop);
		connect(authorLabel, SIGNAL(clicked()), SLOT(authorMoreSlot()));
		
		titleLayout->addWidget(titleLabel);
		titleLayout->addLayout(authorLayout);
		titleLayout->addWidget(honorLabel);
		titleLayout->addWidget(priceLabel);
		titleLayout->addWidget(chapterLab);
		titleLayout->addWidget(publishLabel);

		QHBoxLayout *opLayout = new QHBoxLayout;
		opLayout->setMargin(0);
		opLayout->setSpacing(0);
		butWidget = new EntryButtonWidget(QApplication::font(), this);
		connect(butWidget, SIGNAL(fileDownloaded(const opds::EntryView::FileInfo &)),
			SIGNAL(fileDownloaded(const opds::EntryView::FileInfo &)));
		opLayout->addWidget(butWidget);
		
		titleLayout->addItem(new QSpacerItem(
					margin, margin/2, QSizePolicy::Preferred, QSizePolicy::Fixed));
		titleLayout->addLayout(opLayout);

		baseMsgLayout->addItem(
			new QSpacerItem(margin, 0, QSizePolicy::Preferred, QSizePolicy::Preferred));
		baseMsgLayout->addWidget(thumbnailLabel, 0, Qt::AlignCenter);
		baseMsgLayout->addItem(
			new QSpacerItem(margin/2, margin, QSizePolicy::Preferred, QSizePolicy::Preferred));
		baseMsgLayout->addLayout(titleLayout);
	
		layout->addLayout(baseMsgLayout);
		layout->addItem(new QSpacerItem(margin, margin, QSizePolicy::Preferred, QSizePolicy::Preferred));
		
		int width = (qApp->desktop()->width() - margin*2)/2;
		QVBoxLayout *clickedLayout = new QVBoxLayout;
		clickedLayout->setSpacing(0);
		clickedLayout->setMargin(0);
		QHBoxLayout *clickLabelLayout = new QHBoxLayout;
		clickLabelLayout->setSpacing(0);
		clickLabelLayout->setMargin(0);
		QString pixPath = ZLSApplication::qpeDir() + "pics/obookstore/entry_favorite.png";
		QPixmap pixmap = QPixmap(pixPath);
		
		favoriteLabel = new PictureLabel;
		favoriteLabel->setText(addFavoriteStr);
		favoriteLabel->setPixmap(pixPath);
		favoriteLabel->setFixedSize(width, pixmap.height()+5);
		
		cartLabel = new PictureLabel;
		cartLabel->setText(addCartStr);
		pixPath = ZLSApplication::qpeDir() + "pics/obookstore/shoppingcart.png";
		cartLabel->setPixmap(pixPath);
		cartLabel->setFixedSize(width, pixmap.height()+5);

		PictureLabel *shareLabel = new PictureLabel;
		shareLabel->setText(tr("Share"));
		pixPath = ZLSApplication::qpeDir() + "pics/obookstore/share.png";
		shareLabel->setPixmap(pixPath);
		shareLabel->setFixedSize(width, pixmap.height()+5);
	
		clickLabelLayout->addWidget(shareLabel);
		clickLabelLayout->addWidget(new eink::LineLabel(1, Qt::Vertical));
		connect(shareLabel, SIGNAL(clicked()), SLOT(shareSlot()));
		
		clickLabelLayout->addWidget(favoriteLabel);
		clickLabelLayout->addWidget(new eink::LineLabel(1, Qt::Vertical));
		clickLabelLayout->addWidget(cartLabel);
		clickLabelLayout->addItem(new QSpacerItem(margin, 0, QSizePolicy::Fixed, QSizePolicy::Preferred));
		clickLabelLayout->setAlignment(Qt::AlignHCenter);

		connect(favoriteLabel, SIGNAL(clicked()), SLOT(favoriteSlot()));
		connect(cartLabel, SIGNAL(clicked()), SLOT(cartSlot()));
		
		clickedLayout->addWidget(new eink::LineLabel(1, parent));
		clickedLayout->addItem(new QSpacerItem(margin, margin/8, QSizePolicy::Preferred, QSizePolicy::Fixed));
		clickedLayout->addLayout(clickLabelLayout);
		clickedLayout->addItem(new QSpacerItem(margin, margin/8, QSizePolicy::Preferred, QSizePolicy::Fixed));
		clickedLayout->addWidget(new eink::LineLabel(1, parent));
		layout->addLayout(clickedLayout);
		
		QHBoxLayout *contentLayout = new QHBoxLayout;
		contentLabel = new ContentLabel(dangdang::SMALL_FONT, parent);
		
		QFont smallFont = eink::PainterUtil::getSmallFont();
		QFontMetrics fm(smallFont);
		contentLabel->setFixedHeight(fm.height() * CONTENT_LINE);		
		contentLabel->setWordWrap(true);
		contentLabel->setStyleSheet("a:link { color: #000000; }");
		contentLayout->addItem(new QSpacerItem(margin, margin, QSizePolicy::Fixed, QSizePolicy::Preferred));
		contentLayout->addWidget(contentLabel);
		contentLayout->addItem(new QSpacerItem(margin, margin, QSizePolicy::Fixed, QSizePolicy::Preferred));
		layout->addLayout(contentLayout);

		layout->addItem(new QSpacerItem(margin, margin, QSizePolicy::Preferred, QSizePolicy::Preferred));
		layout->addWidget(new eink::LineLabel(1, parent));
		layout->addItem(new QSpacerItem(margin, margin, QSizePolicy::Preferred, QSizePolicy::Preferred));
		connect(contentLabel, SIGNAL(clicked()), SLOT(collapseSlot()));

		QVBoxLayout *alsoBuyLayout = new QVBoxLayout;
		alsoBuyLayout->setSpacing(0);
		alsoBuyLayout->setMargin(0);
		QHBoxLayout *alsoBuyLabLayout = new QHBoxLayout;
		alsoBuyLabLayout->setSpacing(0);
		alsoBuyLabLayout->setMargin(0);
		dangdang::ContentLabel *alsoBuyLabel = new dangdang::ContentLabel(dangdang::BOLD_LARGE, parent);
		alsoBuyLabel->setText(tr("Also busy"));
		alsoBuyLabLayout->addItem(new QSpacerItem(margin, margin, QSizePolicy::Preferred, QSizePolicy::Preferred));
		alsoBuyLabLayout->addWidget(alsoBuyLabel);
		PictureLabel *refreshLab = new PictureLabel(1);
		refreshLab->setText(tr("Replace"));
		pixPath = ZLSApplication::qpeDir() + "pics/obookstore/refresh.png";
		refreshLab->setPixmap(pixPath);
		alsoBuyLabLayout->addWidget(refreshLab, 0, Qt::AlignRight);
		alsoBuyLabLayout->addItem(new QSpacerItem(margin, margin, QSizePolicy::Preferred, QSizePolicy::Preferred));
		
		QSettings cfg(opds::BookStoreUtil::getInstance()->getDefaultConfig(), QSettings::IniFormat);
		cfg.beginGroup("view");
		int alsoBuyColumn = cfg.value("alsobuycolumn", 3).toInt();
		cfg.endGroup();
		alsoBuyView = new opds::ThumbnailView(opds::BookStoreUtil::getInstance()->getDefaultThumbnailPixmap(), alsoBuyColumn, parent);
		
		alsoBuyView->setShowPageNumber(true);	
		alsoBuyView->setDownloadItemPriorityHigh(true);
		alsoBuyLayout->addLayout(alsoBuyLabLayout);
		alsoBuyLayout->addItem(new QSpacerItem(margin, margin/2, QSizePolicy::Preferred, QSizePolicy::Fixed));
		alsoBuyLayout->addWidget(alsoBuyView, 0, Qt::AlignVCenter);
		connect(alsoBuyView, SIGNAL(selected(const opds::Entry &)),
			SLOT(alsoBuySelected(const opds::Entry &)));
		connect(refreshLab, SIGNAL(clicked()),  SLOT(refreshAlsoBuySlot()));
		layout->addLayout(alsoBuyLayout);

		labelList.append(authorLabel);
		labelList.append(priceLabel);
		labelList.append(chapterLab);
		labelList.append(contentLabel);
		labelList.append(alsoBuyLabel);
		labelList.append(publishLabel);
		changeFont();	
		initCommands();
		
		opds::ImageUtil::getInstance()->attach(this);
		dangdangv5::ShoppingCartManager::getInstance()->attach(this);
		dangdangv5::FavoriteManager::getInstance()->attach(this);
	}

	BookDetailView::~BookDetailView()
	{
		opds::ImageUtil::getInstance()->detach(this);
		dangdangv5::ShoppingCartManager::getInstance()->detach(this);
		dangdangv5::FavoriteManager::getInstance()->detach(this);
	}
	
	void BookDetailView::reset()
	{
		qDebug() << "BookDetailView" << __func__ << "book detail view reset !!!!!!";
		authorLabel->setText("");
		priceLabel->setText("");
		favoriteFinished(false);
		favoriteLabel->setEnabled(true);
		cartLabel->setText(addCartStr);
		cartLabel->setEnabled(true);
		butWidget->reset();
	}
	
	void BookDetailView::clear()
	{
		entry.clear();
		historyEntry.clear();
	}

	bool BookDetailView::doOperate(QKeyEvent *e)
	{
		if (e->type() != QEvent::KeyRelease)
			return false;
		
		switch (e->key()) {
			case Qt::Key_PageUp:
				alsoBuyView->pageUp();
				return true;
			case Qt::Key_PageDown:
				alsoBuyView->pageDown();
				return true;
			case Qt::Key_Escape:
			{
				if (!historyEntry.isEmpty()) {
					back();
					return true;
				}
			}
			default:
				return false;
		}
	}

	void BookDetailView::update(TSubject *s)
	{
		ShoppingCartManager *shoppingMgr = dangdangv5::ShoppingCartManager::getInstance();
		FavoriteManager *favoriteMgr = dangdangv5::FavoriteManager::getInstance();
		if (s == opds::ImageUtil::getInstance()) {
			imgDownloaded(opds::ImageUtil::getInstance()->getDownloadedUrl());
		} else if (s == shoppingMgr) {
			if (shoppingMgr->getOperateEntry().id.isEmpty() || 
					shoppingMgr->getOperateEntry().id != entry.id) 
				return;
			cartLabel->setEnabled(true);
			switch (shoppingMgr->getType()) {
			case dangdangv5::ShoppingCartManager::ADDED:
			{
				if (shoppingMgr->getOperateEntry().id == entry.id) 
					cartLabel->setText(removeCartStr);
				break;
			}
			case dangdangv5::ShoppingCartManager::REMOVED:
			{
				if (shoppingMgr->getOperateEntry().id == entry.id) {
					cartLabel->setText(addCartStr);		
				}  
				break;
			}
			case dangdangv5::ShoppingCartManager::ADD_FAILURE:
			{
				if (dangdangv5::ShoppingCartManager::CART_FULL
				    == shoppingMgr->getAddedStatusCode()) {		
					cartLabel->setText(addCartFail);		
					cartLabel->setEnabled(false);
				} else if (dangdangv5::ShoppingCartManager::EXISTS_V5
						== shoppingMgr->getAddedStatusCode()) {
					cartLabel->setText(removeCartStr);
				}
				break;
			}
			default:
				break;
			}
		} else if (s == favoriteMgr) {
			if (favoriteMgr->getOperateEntry().id.isEmpty() || 
					favoriteMgr->getOperateEntry().id != entry.id) 
				return;
			favoriteLabel->setEnabled(true);
			switch (dangdangv5::FavoriteManager::getInstance()->getType()) {
			case dangdangv5::FavoriteManager::ADD_FAILURE:
			{
				if (dangdangv5::FavoriteFeedRequest::REPEAT == favoriteMgr->getAddedStatusCode())
					favoriteFinished(true);
				else
					favoriteFinished(false);
				break;
			}
			case dangdangv5::FavoriteManager::ADDED:
				favoriteFinished(true);
				break;
			case dangdangv5::FavoriteManager::REMOVED:
				favoriteFinished(false);
				break;
			case dangdangv5::FavoriteManager::REMOVE_FAILURE:
			{
				if (dangdangv5::FavoriteManager::NOT_STORE == favoriteMgr->getRemovedStatusCode()) {
					favoriteFinished(false);
				}
				break;
			}
			default:
				break;
			}
		}
	}

	void BookDetailView::setAuthorLabelEnabled(bool b)
	{
		authorLabel->setEnabled(b);
	}
	
	void BookDetailView::setEntry(const opds::Entry &e)
	{
		if (!e.isValid() || e.id.isEmpty()) {
			qDebug() << "BookDetailView" << __func__ << "WARNING::entry is not Valid !!!!!!!!!";
			return;
		}
		
		if (eink::SystemManager::getInstance()->isDebug())
			opds::OPDSUtil::printEntry(e);
		
		if (entry.id != e.id && !entry.id.isEmpty()) {
			for (int i=0; i<historyEntry.count(); ++i) {
				if (e.id == historyEntry[i].id) {
					historyEntry.removeAt(i);
					break;
				}
			}
			
			if (!entry.id.trimmed().isEmpty()) {
				historyEntry.append(entry);
			}
		}
		
		if (entry.id != e.id)
			emit entryChanged(e);
		reset();
		entry = e;
		titleLabel->setText(entry.title);
		thumbnailLabel->setEntry(entry);
		
		drawAuthor();
		drawTitle();
		handlerPriceLabel();
		handleOriginalBook();
		honorLabel->setHonor(dangdang::Util::getHonor(entry));

		if (!entry.content.isEmpty()) 
			showFixContent();
			
		if (!entry.dcPublisher.isEmpty())
			drawPublisher();

		butWidget->setEntry(entry);
		executeCommands(entry);
		
		if (dangdangv5::ShoppingCartManager::getInstance()->exists(entry)) {
			cartLabel->setText(removeCartStr);
		}
	}

	void BookDetailView::setDetailEntry(const opds::Entry &e)
	{
		if (!e.isValid() || e.id.isEmpty()) {
			qDebug() << "BookDetailView" << __func__ << "WARNING::entry is not valid !!!!!!!";
			return;
		}
		
		if (eink::SystemManager::getInstance()->isDebug())
			opds::OPDSUtil::printEntry(e);
		entry = e;

		drawTitle();
		drawPublisher();
		handleOriginalBook();
		thumbnailLabel->setEntry(entry);
		butWidget->setDetailEntry(entry);
		emit getDetailEntry(entry);

		if (!chapterLab->isHidden()) {
			chapterLab->setText(getLastChapter());
		}
		
		if (!priceLabel->isHidden()) {
			handlerPriceLabel();
		}
		
		if (contentLabel->text().isEmpty())
			showFixContent();
		
		honorLabel->setHonor(dangdang::Util::getHonor(entry));
		if (authorLabel->text().isEmpty())
			drawAuthor();
		if (isOriginBook()) {
			cartLabel->setEnabled(false);
		} else {
			if (hasBuy() || isFreeBook() || isSupportBook() || isPresetBook())
				cartLabel->setEnabled(false);
			else
				cartLabel->setEnabled(true);
		}

		if (hasStore())
			favoriteFinished(true);
		else 
			favoriteFinished(false);
	}
	
	void BookDetailView::handlerPriceLabel()
	{
		int price = getIntPrice(entry);
		QString priceInfo = getPrice();
		if (!priceInfo.isEmpty() && price > 0) {
			priceLabel->setText(priceInfo);
		} else if (price == -1) {
			entry.links.remove(entry.getLink(opds::ACQ_BUY));
		}
	}

	void BookDetailView::alsoBuySelected(const opds::Entry &e) 
	{
		setEntry(e);
	}
	
	void BookDetailView::favoriteSlot()
	{
		if (!EntryView::checkLogin())
			return;
		if(addFavoriteStr == favoriteLabel->text()) {
			dangdangv5::FavoriteManager::getInstance()->addEntry(entry);
		} else if (removeFavoriteStr == favoriteLabel->text()) {
			dangdangv5::FavoriteManager::getInstance()->removeEntry(entry);
		}
		favoriteLabel->setEnabled(false);
	}
	
	void BookDetailView::cartSlot()
	{
		if (!EntryView::checkLogin())
			return;
		if(addCartStr == cartLabel->text()) {
			dangdangv5::ShoppingCartManager::getInstance()->addEntry(entry);
		} else if(removeCartStr == cartLabel->text()){
			dangdangv5::ShoppingCartManager::getInstance()->removeEntry(entry);
		}
		cartLabel->setEnabled(false);
	}
	
	void BookDetailView::shareSlot()
	{
		QTimer::singleShot(400, this, SLOT(delayShowSlot()));
	}
	
	void BookDetailView::delayShowSlot()
	{
		QUrl shareUrl("http://e.dangdang.com/touch/fenxiang/product/product.html");
		shareUrl.addQueryItem("id", entry.id);
		shareUrl.addQueryItem("mediaId", entry.subTitle);
		if (!entry.getLink(opds::ACQ_BUY).isNull()) {
			shareUrl.addQueryItem("mediaTyp", entry.getLink(opds::ACQ_BUY).userData.value("mediaType"));
		}
		qDebug() << "BookDetailView" << __func__ << "shareUrl" << shareUrl.toString();
		qrcode::QRCodeDialog::showUrl(shareUrl);
	}

	void BookDetailView::favoriteFinished(bool success)	
	{
		QString pixPath = ZLSApplication::qpeDir() + "pics/obookstore/entry_favorited.png";
		if (success) {
			favoriteLabel->setText(removeFavoriteStr);
			favoriteLabel->setPixmap(pixPath);
		} else {
			pixPath = ZLSApplication::qpeDir() + "pics/obookstore/entry_favorite.png";
			favoriteLabel->setText(addFavoriteStr);
			favoriteLabel->setPixmap(pixPath);
		}
	}
	
	void BookDetailView::authorMoreSlot()
	{
		QLabel *label = qobject_cast<QLabel *>(sender());
		if (!label)
			return;
		QString author = label->text();
		int pos = author.indexOf(SEPERATOR);
		if (pos != -1) {
			author = author.mid(pos+2);
			emit openAuthor(author);
		}
	}
	
	void BookDetailView::collapseSlot()
	{
		emit showDetailContent(getContent());
	}
	
	void BookDetailView::refreshAlsoBuySlot()
	{
		alsoBuyCommand->refresh(entry);
	}
	
	void BookDetailView::imgDownloaded(const QString &url)
	{
		if (url == entry.getThumnail().href) {
			QString fn = opds::ImageUtil::getInstance()->getSavedFile(url);
			thumbnailLabel->setPixmap(fn);
			butWidget->thumbnailLoaded(url, fn);
		}
	}

	void BookDetailView::initCommands()
	{
		requestList.append(new dangdangv5::BookInfoCommand(this));
		alsoBuyCommand = new dangdangv5::AlsoBuyCommand(alsoBuyView);
		requestList.append(alsoBuyCommand);
	}
	
	void BookDetailView::back()
	{
		qDebug() << "BookDetailView" << __func__ << "entry.title" << entry.title << "historyEntry.count()" << historyEntry.count();
		if (!historyEntry.isEmpty()) {
			entry = historyEntry.takeLast();
			setEntry(entry);
		}
	}
	
	void BookDetailView::drawAuthor()
	{
		QStringList authors = getAuthors();
		qDebug() << "BookDetailView" << __func__ << authors << "authors" << authors;
		if (!authors.isEmpty()) {
			QString authorsText = tr("Author") + SEPERATOR + authors.join(",");
			int width = (authorLabel->width() > qApp->desktop()->width()*2/3) ? authorLabel->width() : qApp->desktop()->width()*2/3;
			qDebug() << "dangdang::EntryView" << __func__ << "width" << width;			
			authorLabel->setText(eink::StringUtil::fitShowWidth(authorsText, width, NULL));
			if (authorLabel->isHidden())
				authorLabel->show();
		} else {
			authorLabel->hide();
		}
	}

	void BookDetailView::drawPublisher()
	{
		if (entry.dcPublisher.isEmpty()) {
			publishLabel->hide();
		} else {
			int width = (publishLabel->width() > qApp->desktop()->width()*2/3) ? publishLabel->width() : qApp->desktop()->width()*2/3;
			QString publisher = tr("Publisher") + SEPERATOR + entry.dcPublisher;
			publishLabel->setText(eink::StringUtil::fitShowWidth(publishLabel->font(), 
						publisher, width, NULL));
			if (publishLabel->isHidden())
				publishLabel->show();
		}
	}

	void BookDetailView::drawTitle()
	{
		int width = titleLabel->width() > qApp->desktop()->width()*2/3 ? titleLabel->width() : qApp->desktop()->width()*2/3;
		titleLabel->setText(eink::StringUtil::fitShowWidth(titleLabel->font(), entry.title, width, NULL));
	}
	
	void BookDetailView::changeFont()
	{
		QFont boldLargeFont(QApplication::font());
		boldLargeFont.setBold(true);
		QFont boldNormalFont(QApplication::font());
		boldNormalFont.setPointSize(boldNormalFont.pointSize() - 2);
		boldNormalFont.setBold(true);
		QFont normalFont(QApplication::font());
		normalFont.setPointSize(normalFont.pointSize() - 3);
		QFont smallFont = eink::PainterUtil::getSmallFont();
		QMap<dangdang::FontType, QFont> fontsMap;
		fontsMap[dangdang::BOLD_LARGE] = boldLargeFont;
		fontsMap[dangdang::BOLD_NORMAL] = boldNormalFont;
		fontsMap[dangdang::NORMAL_FONT] = normalFont;
		fontsMap[dangdang::SMALL_FONT] = smallFont;
		
		butWidget->setFont(boldNormalFont);
		for (int i=0; i < labelList.count(); ++i) {
			dangdang::ClickLabel *label = qobject_cast<dangdang::ClickLabel*>(labelList[i]);
			if (label) {
				label->setFont(fontsMap[label->getFontType()]);
				continue;
			} 
			
			ClickLabel *labelv5 = qobject_cast<ClickLabel*>(labelList[i]);
			if (labelv5) {
				labelv5->setFont(fontsMap[labelv5->getFontType()]);
				continue;
			} 

			dangdang::ContentLabel *clabel = qobject_cast<dangdang::ContentLabel*>(labelList[i]);
			if (clabel) {
				clabel->setFont(fontsMap[clabel->getFontType()]);
				continue;
			}
			
			ContentLabel *clabelv5 = qobject_cast<ContentLabel*>(labelList[i]);
			if (clabelv5) {
				clabelv5->setFont(fontsMap[clabelv5->getFontType()]);
				continue;
			}
		}
	}

	const QString BookDetailView::getPrice()
	{
		QString ret;
		const opds::Link &freeLink = entry.getLink(opds::ACQ_OPEN_ACCESS);
		const opds::Link &priceLink = entry.getLink(opds::ACQ_BUY);
		if (priceLink.isNull()) {
			return entry.opdsPrice;
		} else {
			int price = dangdang::Util::getPrice(entry);
			QString salePrice;
			if (price >= 0)
				salePrice = opds::OPDSUtil::getCurrency(QString::number(price));

			QString paperPrice = opds::OPDSUtil::getCurrency(priceLink.info.prices[opds::Tag::PAPER_BOOK_PRICE]);
			if (!paperPrice.isEmpty()) {
				ret.append("<s>" + tr("Paper Book Price") + SEPERATOR + paperPrice + "</s>" + QString("        ")); 
			}
			
			if (freeLink.isNull()) {
				ret.append(tr("Price") + SEPERATOR + salePrice);
			} else {
				ret.append(tr("Price") + SEPERATOR + tr("Free"));
			}
			
			ret.prepend("<pre>");
			ret.append("</pre>");
			return ret;
		}
	}
	
	int BookDetailView::getIntPrice(const opds::Entry &e)
	{
		const opds::Link &priceLink = e.getLink(opds::ACQ_BUY);
		if (priceLink.isNull()) 
			return e.opdsPrice.toInt();
		return dangdang::Util::getPrice(e);
	}

	const QString BookDetailView::getContent()
	{
		QString ret;
		if (!entry.content.isEmpty()) 
			ret = entry.content.trimmed();
		else if (!entry.summary.isEmpty())
			ret = entry.summary.trimmed();
		else 
			ret = QString::null;
		QTextDocument doc;
		doc.setHtml(ret);
		ret = doc.toPlainText();
		ret.replace("\n", "<br>");
		return ret;
	}

	const QString BookDetailView::getLastChapter()
	{
		const opds::Link &link = entry.getLink(opds::REL_ALTERNATE);
		QString lastChapter;
		if (!link.isNull()) {
			lastChapter = link.userData.value("lastChapterName");
			if (!lastChapter.isEmpty()) {
				lastChapter.prepend(SEPERATOR);
				lastChapter.prepend(tr("LastChapterName"));
			}
		}
		int width = qApp->desktop()->width()*2/3;
		return eink::StringUtil::fitShowWidth(chapterLab->font(), lastChapter, width, NULL);
	}

	const QStringList BookDetailView::getAuthors()
	{
		QStringList ret;
		std::list<opds::Author>::const_iterator it = entry.authors.begin();
		for (; it != entry.authors.end(); ++it) {
			ret.append(it->name);
		}
		return ret;
	}
	
	void BookDetailView::handleOriginalBook()
	{
		QString originMediaId = dangdang::Util::getOriginMediaId(entry);
		if (entry.id == originMediaId || originMediaId.isEmpty()) {
			priceLabel->show();
			chapterLab->hide();
		} else {
			priceLabel->hide();
			chapterLab->show();
		}
	}
	
	void BookDetailView::showFixContent()
	{
		int w = qApp->desktop()->width() - 2 * eink::SystemManager::getInstance()->getMargin();
		bool exceed;
		QFont smallFont = eink::PainterUtil::getSmallFont();
		QFontMetrics fm(smallFont);
		QString str = eink::StringUtil::fitShowWidth(getContent(), w * CONTENT_LINE, &exceed);
		if (!str.isEmpty()) {
			contentLabel->setText(str);
			contentLabel->setAlignment(Qt::AlignTop);
		}
		
		if (!exceed) {
			contentLabel->setEnabled(false);
		} else {
			contentLabel->setEnabled(true);
		}
	}

	void BookDetailView::executeCommands(const opds::Entry &entry)
	{
		for (int i=0; i<requestList.count(); ++i) {
			requestList[i]->execute(entry);
		}
	}
	
	bool BookDetailView::isFreeBook()
	{
		const opds::Link &freeLink = entry.getLink(opds::ACQ_OPEN_ACCESS);
		if (!freeLink.isNull()) {
			return true;
		}
		return false;
	}
	
	bool BookDetailView::isSupportBook()
	{
		return !dangdang::Util::getInstance()->isSupportDeviceBook(entry);	
	}

	bool BookDetailView::isPresetBook()
	{
		return dangdang::PresetBookManager::getInstance()->isProductIdExists(entry.id);
	}

	bool BookDetailView::hasStore()
	{
		opds::Link link = entry.getLink(opds::ACQ_BUY);	
		if (!link.isNull()) 
			return link.userData.value(BookInfoParser::IS_STORE) == opds::Tag::OPDS_TRUE;
		else
			return false;
	}

	bool BookDetailView::hasBuy()
	{
		return dangdang::Util::isBuy(entry);
	}
	
	bool BookDetailView::isOriginBook()
	{
		if (!entry.getLink(opds::ACQ_BUY).isNull()) {
			return opds::Tag::ORIGINAL_BOOK == 
				entry.getLink(opds::ACQ_BUY).userData.value("mediaType");
		}
		return false;
	}
	
	//class ContentView
	ContentView::ContentView(QWidget *parent)
		:QWidget(parent)
	{
		delegate = NULL;
		QVBoxLayout *layout = new QVBoxLayout(this);
		chapterLabel = new dangdang::ContentLabel(dangdang::BOLD_NORMAL, this);
		chapterLabel->setText(tr("Sorry, Chapter is Empty!"));
		chapterLabel->setAlignment(Qt::AlignCenter);
		contentListView = new ContentListView(opds::BookStoreUtil::getInstance()->getNetworkMgr());
		initFeedView(contentListView);

		stack = new QStackedWidget(this);
		stack->addWidget(chapterLabel);
		stack->addWidget(contentListView);
		stack->setCurrentWidget(contentListView);
		layout->addWidget(stack);
	}

	ContentView::~ContentView()
	{
		if (delegate != NULL) {
			delete delegate;
			delegate = NULL;
		}
	}

	void ContentView::loadEntry(const opds::Entry &entry)
	{
		qDebug() << "ContentView" << __func__ << "begin load chapterList !!!!!";
		stack->setCurrentWidget(contentListView);
		contentListView->loadEntry(entry);
	}

	void ContentView::dataLoaded()
	{
		qDebug() << "ContentView" << __func__ << "load chapterList finished !!!!!";
		if (contentListView->isEmpty()) {
			stack->setCurrentWidget(chapterLabel);
		}
	}
	
	void ContentView::initFeedView(opds::FeedView *view, bool setDelegate)
	{
		connect(view, SIGNAL(dataLoaded()), this, SLOT(dataLoaded()));
		view->setItemHeight(opds::BookStoreUtil::getInstance()->getBookPixmap()->height());
		view->setShowInfo(false);
		view->setHasPageMgr(false);
		view->setInfoLabelVisible(false);
		if (delegate == NULL)
			delegate = new FeedViewDelegate(this);
		if (setDelegate)
			view->setDelegate(delegate);
		view->setLayoutSpacing(1);
		view->setLayoutMargin(1);
	}

	//class CommentView
	CommentView::CommentView(QWidget *parent)
		:QWidget(parent)
	{
		delegate = NULL;
		QVBoxLayout *layout = new QVBoxLayout(this);
		layout->setMargin(0);
		layout->setSpacing(0);
		const int margin = eink::SystemManager::getInstance()->getMargin();

		dangdang::ContentLabel *userLabel = new dangdang::ContentLabel(dangdang::BOLD_NORMAL);
		userLabel->setText(tr("User comments"));
		dangdang::ClickLabel *addCommentLabel = new dangdang::ClickLabel(dangdang::BOLD_NORMAL);
		addCommentLabel->setText(tr("Add Comment"));
		connect(addCommentLabel, SIGNAL(clicked()), this, SIGNAL(addComment()));
		
		QHBoxLayout *labLayout = new QHBoxLayout;
		labLayout->setMargin(0);
		labLayout->setSpacing(0);
		labLayout->addItem(new QSpacerItem(margin*3, margin, QSizePolicy::Preferred, QSizePolicy::Preferred));
		labLayout->addWidget(userLabel);
		labLayout->addItem(new QSpacerItem(margin, margin, QSizePolicy::Expanding, QSizePolicy::Preferred));
		labLayout->addWidget(addCommentLabel);
		labLayout->addItem(new QSpacerItem(margin*3, margin, QSizePolicy::Preferred, QSizePolicy::Preferred));

		commentLabel = new dangdang::ContentLabel(dangdang::BOLD_NORMAL, this);
		commentLabel->setText(tr("Sorry, Comment is Empty!"));
		commentLabel->setAlignment(Qt::AlignCenter);
		commentListView = new CommentListView(opds::BookStoreUtil::getInstance()->getNetworkMgr());
		initFeedView(commentListView);

		stack = new QStackedWidget;
		stack->addWidget(commentLabel);
		stack->addWidget(commentListView);
		stack->setCurrentWidget(commentListView);
		layout->addLayout(labLayout);
		layout->addWidget(stack);
		command = new BookCommentCommand(this);
	}

	CommentView::~CommentView()
	{
		if (delegate != NULL) {
			delete delegate;
			delegate = NULL;
		}
	}

	void CommentView::loadEntry(const opds::Entry &entry)
	{
		qDebug() << "CommentView" << __func__ << "begin load commetList !!!!!";
		stack->setCurrentWidget(commentListView);
		command->execute(entry);
	}
	
	void CommentView::loadComment(const opds::Entry &entry)
	{
		stack->setCurrentWidget(commentListView);
		commentListView->loadEntry(entry);
	}

	void CommentView::showEmpty()
	{
		stack->setCurrentWidget(commentLabel);
	}

	void CommentView::dataLoaded()
	{
		qDebug() << "CommentView" << __func__ << "load commentList finished !!!!!";
		if (commentListView->isEmpty()) {
			stack->setCurrentWidget(commentLabel);
		}
	}
	
	void CommentView::initFeedView(opds::FeedView *view, bool setDelegate)
	{
		connect(view, SIGNAL(openEntry(const opds::Entry &)),
			this, SIGNAL(openCommentEntry(const opds::Entry &)));
		connect(view, SIGNAL(dataLoaded()), this, SLOT(dataLoaded()));
		
		view->setItemHeight(opds::BookStoreUtil::getInstance()->getBookPixmap()->height());
		view->setShowInfo(false);
		view->setHasPageMgr(false);
		view->setInfoLabelVisible(false);
		if (delegate == NULL)
			delegate = new FeedViewDelegate(this);
		if (setDelegate)
			view->setDelegate(delegate);
		view->setLayoutSpacing(1);
		view->setLayoutMargin(1);
	}

}

