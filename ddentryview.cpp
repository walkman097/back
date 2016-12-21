#include "ddentryview.h"
#include "buttonmanager.h"
#include "httpdownloader.h"
#include "zlsbrowser.h"
#include "bookstoreutil.h"
#include "einklocal.h"
#include "xxsdir.h"
#include "zlsapplication.h"
#include "systemmanager.h"
#include "stringutil.h"
#include "imageutil.h"
#include "opdsutil.h"
#include "label.h"
#include "popupmenu.h"
#include "stringutil.h"
#include "scrollarea.h"
#include "painterutil.h"
#include "ddentrycommand.h"
#include "commentlistview.h"
#include "dangdangutil.h"
#include "bookstorefactory.h"
#include "shoppingcartmanager.h"
#include "favoritemanager.h"
#include "opds/thumbnailview.h"
#include "opds/constants.h"
#include "opds/imageutil.h"
#include "ddentrybuttonwidget.h"
#include "ddshoppingcartmanager.h"
#include "ddfavoritemanager.h"
#include "einkmediawidget.h"
#include "ddcloudmanager.h"
#include "entrythumbnaillabel.h"
#include <QRegExp>
#include <QtGui>

#define THUMBNAIL_PIXMAP ("/tmp/obookstore_thumbnail")
#define SEPERATOR (": ")
#define FULL_HONOR (5)
#define NOT_LAYOUT_WIDTH (100)
#define DANGDANGV5     ("dangdang5.0")

namespace dangdang {

	EntryView::EntryView(QWidget *parent)
		:opds::EntryView(parent)
		,commentView(NULL)
		,price(-1)
	{
		QStringList qms;
		qms << "libopds";
		ZLSApplication::loadTranslations(qms);

		eink::EinkLocal::getInstance()->setCodecLocal();

		QVBoxLayout *layout = new QVBoxLayout(this);
		scrollArea = new eink::ScrollArea(this);
		scrollArea->setWidget(getWidget());
		layout->addWidget(scrollArea);

		initCommands();
		opds::ImageUtil::getInstance()->attach(this);
		
		loginManager = opds::BookStoreUtil::getInstance()->factory()->getLoginManager();
		dangdang::ShoppingCartManager::getInstance()->attach(this);
		dangdang::FavoriteManager::getInstance()->attach(this);
	}

	EntryView::~EntryView()
	{
		dangdang::ShoppingCartManager::getInstance()->detach(this);		
		dangdang::FavoriteManager::getInstance()->detach(this);		
		opds::ImageUtil::getInstance()->detach(this);
		if (alsoBuyDefaultPixmap)
			delete alsoBuyDefaultPixmap;
	}
	
	void EntryView::clear()
	{
		entry.clear();
	}

	void EntryView::reload()
	{
		if (entry.isValid())
			setEntry(entry);
	}
	
	void EntryView::update(TSubject *s)
	{
		dangdang::ShoppingCartManager *shoppingMgr = dangdang::ShoppingCartManager::getInstance();
		if (s == opds::ImageUtil::getInstance()) {
			imgDownloaded(opds::ImageUtil::getInstance()->getDownloadedUrl());
		} else if (s == shoppingMgr) {
			qDebug() << "dangdang::EntryView" << __func__ << "getOperateEntry().id" <<
				shoppingMgr->getOperateEntry().id << "entry.id" << entry.id;
			if (shoppingMgr->getOperateEntry().id != entry.id)
				return;
			switch (shoppingMgr->getType()) {
			case dangdang::ShoppingCartManager::ADDED:
			{
				if (shoppingMgr->getOperateEntry().id == entry.id) 
					cartLabel->setText(removeCartStr);
				break;
			}
			case dangdang::ShoppingCartManager::REMOVED:
			{
				if (shoppingMgr->getOperateEntry().id == entry.id) {
					cartLabel->setText(addCartStr);		
				}  
				break;
			}
			case dangdang::ShoppingCartManager::ADD_FAILURE:
			{
				if (dangdang::ShoppingCartManager::CART_FULL
				    == shoppingMgr->getAddedStatusCode()) {		
					cartLabel->setText(addCartFail);		
				} else if (100 == shoppingMgr->getAddedStatusCode()) {
					cartLabel->setText(removeCartStr);				
				}
				break;
			}
			default:
				break;
			}
		} else if (s == dangdang::FavoriteManager::getInstance()) {			
			switch (dangdang::FavoriteManager::getInstance()->getType()) {
			case dangdang::FavoriteManager::ADDED:
			{
				if (dangdang::FavoriteManager::getInstance()->getOperateEntry().id == entry.id) {
					favoriteLabel->setText(removeFavoriteStr);
				}
				break;
			}
			case dangdang::FavoriteManager::REMOVED:
			{
				if (dangdang::FavoriteManager::getInstance()->getOperateEntry().id == entry.id)
					favoriteLabel->setText(addFavoriteStr);		
				break;
			}
			default:
				break;
			}
		} 
	}

	void EntryView::reset()
	{
		titleLabel->setText("");
		authorLabel->setText("");
		priceLabel->setText("");
		langLabel->setText("");
		favoriteLabel->setText(addFavoriteStr);
		cartLabel->setText(addCartStr);
		scrollArea->verticalScrollBar()->triggerAction(QAbstractSlider::SliderToMinimum);
		butWidget->reset();
	}

	void EntryView::imgDownloaded(const QString &url)
	{
		if (url == entry.getThumnail().href) {
			QString fn = opds::ImageUtil::getInstance()->getSavedFile(url);
			//QPixmap pixmap(fn);
			//thumbnailLabel->setPixmap(pixmap);
			thumbnailLabel->setPixmap(fn);
			butWidget->thumbnailLoaded(url, fn);
		}
	}

	void EntryView::setEntry(const opds::Entry &e)
	{
		if (eink::SystemManager::getInstance()->isDebug())
			opds::OPDSUtil::printEntry(e);
		reset();
		entry = e;
		price = getIntPrice(e);
		
		contentText = getContent();		

		thumbnailLabel->setEntry(e);
		
		drawTitle(entry.title);
		drawAuthor();

		QString priceInfo = getPrice();
		qDebug() << "EntryView " << __func__ << "price" << price << priceInfo;
		if (!priceInfo.isEmpty()) {
			priceLabel->setText(priceInfo);
		}
		honorLabel->setHonor(dangdang::Util::getHonor(entry));

		if (eink::SystemManager::getInstance()->isDebug())
			opds::OPDSUtil::printEntry(e);

		if (!entry.content.isEmpty()) 
			showDetailContent();
		//showSimpleContent();
		
		setInfo(publisherLabel, tr("Publisher"), entry.dcPublisher);
		setInfo(langLabel, tr("Language"), entry.dcLanguage);
	
		setDate(createdLabel, tr("Created"), entry.updated);
		setDate(publishDateLabel, tr("Published"), entry.published);

		executeCommands(entry);
		//pronounce(entry);

		//butWidget->reset();
		butWidget->setEntry(entry);
		
		if (dangdang::ShoppingCartManager::getInstance()->exists(entry)) {
			cartLabel->setText(removeCartStr);
		}
		if (dangdang::FavoriteManager::getInstance()->exists(entry)) {
			favoriteLabel->setText(removeFavoriteStr);
		}
	}

	void EntryView::setDetailEntry(const opds::Entry &e)
	{
		if (eink::SystemManager::getInstance()->isDebug())
			opds::OPDSUtil::printEntry(e);
		entry = e;
		int p = getIntPrice(e);
		qDebug() << "EntryView " << __func__  << "p" << p << "price" << price;
		bool supported = butWidget->setDetailEntry(entry);
		cartLabel->setEnabled(supported && !(hasBuy() || isFreeBook()));
		favoriteLabel->setEnabled(supported);

		contentText = getContent();

		if (contentLabel->text().isEmpty() || !entry.subTitle.isEmpty())
			showDetailContent();
		//showSimpleContent();
		
		if (!e.dcPublisher.isEmpty() && publisherLabel->isHidden()) {
			setInfo(publisherLabel, tr("Publisher"), entry.dcPublisher);
		} 
		if (entry.published != 0 && publishDateLabel->isHidden()) {
			setDate(publishDateLabel, tr("Published"), entry.published);
		}
		if (!entry.dcLanguage.isEmpty() && langLabel->isHidden()) {
			qDebug() << "EntryView"  << __func__ << "language "<<e.dcLanguage;
			setInfo(langLabel, tr("Language"), entry.dcLanguage);
		}

		if (p != price && p > 0) {
			QString priceInfo = getPrice();
			qDebug() << "EntryView" << __func__ << "priceInfo" << priceInfo;
			if (!priceInfo.isEmpty()) 
				priceLabel->setText(priceInfo);
		}
		honorLabel->setHonor(dangdang::Util::getHonor(entry));
		if (authorLabel->text().isEmpty())
			drawAuthor();
	}

	void EntryView::drawAuthor()
	{
		QStringList authors = getAuthors();
		qDebug() << "EntryView" << __func__ << authors << "authors.isEmpty()" << authors.isEmpty();
		if (!authors.isEmpty()) {
			QString authorsText = tr("Author") + SEPERATOR + authors.join(",");
			int width = authorLabel->width();
			if (width <= 100 || width > qApp->desktop()->width())
				width = qApp->desktop()->width()  / 2;
			qDebug() << "dangdang::EntryView" << __func__ << "width" << width;			
			authorLabel->setText(eink::StringUtil::fitShowWidth(authorsText, width, NULL));
		} else {
			authorLabel->setText("");
		}
	}
	
	void EntryView::pronounce(const opds::Entry &entry)
	{
		QString text = tr("Open:") + entry.title;
		if (!getAuthors().isEmpty())
			text += "." + getAuthors().join(",");
		QString priceInfo = getPrice();
		if (!priceInfo.isEmpty())
			text += "." + priceInfo;
		if (!entry.dcPublisher.isEmpty())
			text += "." + entry.dcPublisher;
		//text += tr("Up down key select operation");
		text += tr("press Enter to download book!");
		if (!contentText.isEmpty())
			text += "." + tr("Summary") + ":" + contentText;
		eink::StringUtil::pronounce(text);
	}

	bool EntryView::doOperate(QKeyEvent *e)
	{
#if 0
		if (e->type() == QKeyEvent::KeyRelease)
			return butMgr->processKeyEvent(e);
#endif
		return false;
	}

	QStringList EntryView::getAuthors()
	{
		QStringList ret;

		std::list<opds::Author>::const_iterator it = entry.authors.begin();
		for (; it != entry.authors.end(); ++it) {
			ret.append(it->name);
		}
		return ret;
	}

	int EntryView::getIntPrice(const opds::Entry &e)
	{
		const opds::Link &priceLink = e.getLink(opds::ACQ_BUY);
		if (priceLink.isNull()) {
			return e.opdsPrice.toInt();
		} else {
			return dangdang::Util::getPrice(e);
		}
	}

	QString EntryView::getPrice()
	{
		QString ret;

		const opds::Link &freeLink = entry.getLink(opds::ACQ_OPEN_ACCESS);
		const opds::Link &priceLink = entry.getLink(opds::ACQ_BUY);
		const QString seperator = " / ";
		if (priceLink.isNull()) {
			return entry.opdsPrice;
		} else {
			int price = dangdang::Util::getPrice(entry);
			QString salePrice;
			if (price >= 0)
				salePrice = opds::OPDSUtil::getCurrency(QString::number(price));
			QString paperPrice = opds::OPDSUtil::getCurrency(priceLink.info.prices[opds::Tag::PAPER_BOOK_PRICE]);
			if (!paperPrice.isEmpty()) {
				ret.append(tr("Paper Book Price") + SEPERATOR + "<s>" + paperPrice + "&nbsp;</s><br>");
			}
			if (freeLink.isNull())
				ret.append("<b>" + tr("Price") + SEPERATOR + salePrice + "</b>");
			else
				ret.append("<b>" + tr("Price") + SEPERATOR + opds::OPDSUtil::getCurrency("0") + "</b>");
			return ret;
		}
	}
	
	bool EntryView::isFreeBook()
	{
		const opds::Link &freeLink = entry.getLink(opds::ACQ_OPEN_ACCESS);
		if (!freeLink.isNull()) {
			return true;
		}
		return false;
	}

	bool EntryView::hasBuy()
	{
		dangdang::CloudManager *mgr = dynamic_cast<dangdang::CloudManager*>(opds::BookStoreUtil::getInstance()->factory()->getCloudManager());
		BookInfo::Type type = mgr->getType(entry); 
		return type == BookInfo::BUY;
	}

	void EntryView::showSimpleContent()
	{
		int w = qApp->desktop()->width() - 2 * eink::SystemManager::getInstance()->getMargin();
		bool exceed;
		QString str = eink::StringUtil::fitShowWidth(contentText, w * 4, &exceed);
		if (!exceed) {
			collapseButton->hide();
		} else {
			collapseButton->setDirection(eink::PopupMenuButton::DOWN);
			collapseButton->setText(showMoreStr);
			collapseButton->show();
		}
		if (str == "null" || str.isEmpty()) {
			contentLabel->setText(QString::null);
			collapseButton->hide();
		} else {
			contentLabel->setText("<p>" + str + "</p>");
		}
	}

	void EntryView::showDetailContent()
	{
		if (contentText == "null" || contentText.isEmpty())
			contentLabel->setText("");
		else
			contentLabel->setText("<p>" + contentText + "</p>");
#if 0
		collapseButton->setDirection(eink::PopupMenuButton::UP);
		collapseButton->setText(collapseStr);
#endif
	}

	void EntryView::collapseSlot()
	{
		if (collapseButton->getDirection() == eink::PopupMenuButton::UP)
			showSimpleContent();
		else
			showDetailContent();
	}

	QString EntryView::getContent() const
	{
		QString ret;
		if (!entry.content.isEmpty()) 
			ret = entry.content.trimmed();
		else if (!entry.summary.isEmpty())
			ret = entry.summary.trimmed();
		else if( !entry.subTitle.isEmpty())
			ret = entry.subTitle.trimmed();
		else 
			ret = spaceString;
		QTextDocument doc;
		doc.setHtml(ret);
		ret = doc.toPlainText();
		ret = ret.replace("\n", "<br>");
		qDebug() << "EntryView" << __func__ << "ret" << ret;
		const QString seperators = "----------";
		//seperators = eink::StringUtil::fitShowWidth(contentLabel->font(), seperators, width(), NULL);
		if (ret.indexOf(seperators) != -1) {
			ret = ret.replace(seperators, "-");
			qDebug() << "EntryView" << __func__ << "context indexOf seperators != -1!!";
		}
		return ret;
	}

	void EntryView::setInfo(QLabel *label, const QString &title, const QString &info)
	{
		if (info.isEmpty()) {
			label->hide();
		} else {
			label->setText( "<b>" + title + "</b>" + SEPERATOR + info);
			label->show();
		}
	}

	void EntryView::setDate(QLabel *label, const QString &title, long timeT)
	{
		if (timeT <= 0) {
			label->hide();
		} else {
			QString info("<b>" + title + "</b>" + SEPERATOR);
			QDate date = QDateTime::fromTime_t(timeT).date();
			qDebug() << "dangdang::EntryView" << __func__ << "timeT" << timeT << "date" << date << "date.year()" << date.year();
			if (date.year() > 1900) {
				info += QString::number(date.year()) + "-";
				if (date.month() > 0) {
					info += QString::number(date.month());
					info += "-";
					info += QString::number(date.day());
				}
			} else {
				if (date.month() > 0) {
					info += QString::number(date.month());
					info += "-";
					info += QString::number(date.day());
				}
			}
			label->setText(info);
			label->show();
		}
	}

	void EntryView::drawTitle(const QString &title)
	{
		if (titleLabel->width() == NOT_LAYOUT_WIDTH) {
			titleLabel->setText("<b>" + title + "</b>");
			return;
		}

		qDebug() << "EntryView" << __func__ << title << "titleLabel->size()" << titleLabel->size();
		const int margin = eink::SystemManager::getInstance()->getMargin();
		QFont boldLargeFont(QApplication::font());
		boldLargeFont.setBold(true);
		QFontMetrics qfm(boldLargeFont);
		bool exceed = false;
		QRect titleRect(0, 0, titleLabel->width() - margin * 2, qfm.height());
		QString str = eink::StringUtil::fitShowWidth(
			QApplication::font(), title, titleRect.width(), &exceed);
		if (exceed) {
			titleRect.setHeight(qfm.height() * 2);
			str = eink::StringUtil::boundText(
				title, titleRect, QApplication::font());
		}
		titleLabel->setText("<b>" + str + "</b>");
	}
	
	void EntryView::authorMoreSlot()
	{
		//TODO maybe need separate the authors to clicked;
		QLabel *label = qobject_cast<QLabel *>(sender());
		if (!label)
			return;
		QString author = label->text();
		int pos = author.indexOf(SEPERATOR);
		if (pos != -1)
			author = author.mid(pos);
		opds::Link link;
		link.userData[dangdang::Util::ACTION] = "searchBooksByAuthor";
		link.userData["author"] = author;
		link.title = label->text();
		emit openLink(link);
	}

	bool EntryView::checkLogin()
	{
		if (!loginManager->isLogined()) {
			eink::EinkMessageBox box(this);
			box.setMessage(tr("You have not login, please login first."));
			box.setTitle(tr("Login"));
			box.setOkButtonText(tr("Login Now"));
			box.exec();
			if (eink::EinkMessageBox::STATE_OK == box.getState()) {
				ZLSApplication::execute("loginuserinfo");
			}
			return false;
		}
		return true;
	}

	void EntryView::favoriteSlot()
	{
		if (!checkLogin())
			return;
		if(addFavoriteStr == favoriteLabel->text()) {
			addFavorite();
		} else if (removeFavoriteStr == favoriteLabel->text()) {
			deleteFavorite();		
		}
	}

	void EntryView::cartSlot()
	{
		if (!checkLogin())
			return;
		if(addCartStr == cartLabel->text()) {
			addCart();
		} else if(removeCartStr == cartLabel->text()){
			deleteCart();
		}
	}

	void EntryView::addCart()
	{
		dangdang::ShoppingCartManager::getInstance()->addEntry(entry);
	}

	void EntryView::deleteCart()
	{
		dangdang::ShoppingCartManager::getInstance()->removeEntry(entry);
	}


	void EntryView::addFavorite()
	{
		dangdang::FavoriteManager::getInstance()->addEntry(entry);
	}

	void EntryView::deleteFavorite()
	{
		dangdang::FavoriteManager::getInstance()->removeEntry(entry);
	}

	void EntryView::catalogSlot()
	{
		//TODO 
	}

	void EntryView::commentSlot()
	{
		//TODO
	}
	
	void EntryView::moreCommentSlot()
	{
		//TODO
	}
	
	void EntryView::initCommands()
	{
		requestList.append(new dangdang::BookInfoCommand(this));
		if (commentView) 
			requestList.append(new dangdang::BookCommentCommand(commentView));
		requestList.append(new dangdang::AlsoBuyCommand(alsoBuyView));
	}

	void EntryView::executeCommands(const opds::Entry &entry)
	{
		for (int i=0; i<requestList.count(); ++i) {
			requestList[i]->execute(entry);
		}
	}

	void EntryView::alsoBuySelected(const opds::Entry &entry)
	{
		setEntry(entry);
	}

	void EntryView::changeFont()
	{
		QFont boldLargeFont(QApplication::font());
		boldLargeFont.setBold(true);
		QFont boldNormalFont(QApplication::font());
		boldNormalFont.setPointSize(boldNormalFont.pointSize() - 2);
		boldNormalFont.setBold(true);
		QFont normalFont(QApplication::font());
		normalFont.setPointSize(normalFont.pointSize() - 2);
		QFont smallFont = eink::PainterUtil::getSmallFont();
		QMap<FontType, QFont> fontsMap;
		fontsMap[BOLD_LARGE] = boldLargeFont;
		fontsMap[BOLD_NORMAL] = boldNormalFont;
		fontsMap[NORMAL_FONT] = normalFont;
		fontsMap[SMALL_FONT] = smallFont;
		
		for (int i=0; i<labelList.count(); ++i) {
			ClickLabel *label = qobject_cast<ClickLabel*>(labelList[i]);
			if (label) {
				label->setFont(fontsMap[label->getFontType()]);
			} else {
				ContentLabel *clabel = qobject_cast<ContentLabel*>(labelList[i]);
				if (clabel) {
					clabel->setFont(fontsMap[clabel->getFontType()]);
				}
			}
		}
		collapseButton->setFont(boldNormalFont);
		if (catalogButton)
			catalogButton->setFont(boldNormalFont);
		if (commentButton)
			commentButton->setFont(boldNormalFont);
		butWidget->setFont(boldNormalFont);
	}



	//class ClickLabel
	ClickLabel::ClickLabel(FontType ft, QWidget *parent)
		:QLabel(parent)
		,pressed(false)
		,alignment(Qt::AlignLeft)
		,fontType(ft)
	{
	}


	ClickLabel::~ClickLabel()
	{
	}

	FontType ClickLabel::getFontType()
	{
		return fontType;
	}

	void ClickLabel::setAlignment(Qt::Alignment a)
	{
		alignment = a;
	}

	QSize ClickLabel::sizeHint() const
	{
		QFontMetrics fm(font());
		return QSize(fm.width(text()), fm.height());
	}
	
	void ClickLabel::mousePressEvent(QMouseEvent *e)
	{
		pressed = true;
		update();
		QWidget::mousePressEvent(e);
	}
	
	void ClickLabel::mouseReleaseEvent(QMouseEvent *e)
	{
		if (pressed) {
			pressed = false;
			update();
			emit clicked();
		}
		QWidget::mouseReleaseEvent(e);
	}
	
	void ClickLabel::paintEvent(QPaintEvent *)
	{
		QPainter p(this);
		QPen pen(Qt::black);
		QFontMetrics fm(font());
		int w = fm.width(text());
		
		QRect r(rect());
		QLine line;
		if (alignment == Qt::AlignCenter) {
			int x = (width() - w) / 2;
			int y = height() - 2;
			line = QLine(QPoint(x, y), QPoint(width() - x, y));
		} else {
			int x = 0;
			r.setX(x);
			r.setWidth(rect().width() - x);
			int y = fm.height() - 2;
			line = QLine(QPoint(x, y), QPoint(w, y));
		}
		//qDebug() << "ClickLabel" << __func__ << "fm.height()" << fm.height() << "height" << height();

		if (pressed) {
			p.fillRect(rect(), Qt::black);
			pen.setColor(Qt::white);
		} else {
			p.fillRect(rect(), Qt::white);
		}

		p.setPen(pen);
		p.drawText(r, alignment, text());
		if (isEnabled()) {
			p.drawLine(line);
		}
	}


	//class ContentLabel
	ContentLabel::ContentLabel(FontType t, QWidget *parent)
		:QLabel(parent)
		,fontType(t)
	{
	}

	ContentLabel::ContentLabel(FontType t, const QString &txt, QWidget *parent)
		:QLabel(txt, parent)
		,fontType(t)
	{
	}	
	
	ContentLabel::~ContentLabel()
	{
	}

	void ContentLabel::mousePressEvent(QMouseEvent *e)
	{
		QWidget::mousePressEvent(e);
	}

	/** 
	 * The QLabel::mouseReleaseEvent() will accept the release event;
	 * 
	 * @param e 
	 */
	void ContentLabel::mouseReleaseEvent(QMouseEvent *e)
	{
		QWidget::mouseReleaseEvent(e);		
	}

	FontType ContentLabel::getFontType()
	{
		return fontType;
	}


	//class HonorLabel 
	HonorLabel::HonorLabel(QWidget *parent)
		:QWidget(parent)
		,honorPixmap(ZLSApplication::qpeDir() + "/pics/obookstore/honor.png")
		,honorEmptyPixmap(ZLSApplication::qpeDir() + "/pics/obookstore/honor_empty.png")
		,honor(0) {
	}

	HonorLabel::~HonorLabel()
	{
	}

	QSize HonorLabel::sizeHint() const
	{
		return QSize(width(), honorPixmap.height());
	}

	void HonorLabel::setHonor(int h) 
	{
		honor = h;
		update();
	}
	
	void HonorLabel::paintEvent(QPaintEvent *)
	{
		QPainter p(this);
		const int span = 5;
		int x = 0;
		int y = (height() - honorPixmap.height()) / 2;
		for (int i=0; i<FULL_HONOR; ++i) {
			if (i < honor) {
				p.drawPixmap(x, y, honorPixmap);
			} else {
				p.drawPixmap(x, y, honorEmptyPixmap);
			}
			x += honorPixmap.width() + span;
		}
	}
	
	//Because of declare problem, do the getWidget at here;
	QWidget *EntryView::getWidget()
	{
		QWidget *widget = new QWidget(this);
		QWidget *parent = widget;//this;
		
		const int margin = eink::SystemManager::getInstance()->getMargin();
		spaceString = " ";
		collapseStr = tr("Collapse");
		showMoreStr = tr("Show more");
		
		addFavoriteStr = tr("Add favorite");
		removeFavoriteStr = tr("Delete favorite");
		addCartStr = tr("Add cart");
		addCartFail = tr("Shopping cart full");
		removeCartStr = tr("Delete cart");
		
		QVBoxLayout *layout = new QVBoxLayout(parent);
		layout->setMargin(margin / 2);
		
		QHBoxLayout *baseMsgLayout = new QHBoxLayout;

		thumbnailLabel = new opds::EntryThumbnailLabel(parent);
		thumbnailLabel->setAlignment(Qt::AlignCenter);
	
		QVBoxLayout *titleLayout = new QVBoxLayout;
		titleLayout->setMargin(0);
		titleLayout->setSpacing(0);
		
		titleLabel = new ContentLabel(BOLD_LARGE, parent);
		titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
		titleLabel->setWordWrap(true);
		QHBoxLayout *authorLayout = new QHBoxLayout;
		authorLabel = new ClickLabel(NORMAL_FONT, parent);
		authorLabel->setAlignment(Qt::AlignLeft);
		authorLayout->addWidget(authorLabel, 1);
		authorLayout->addItem(
			new QSpacerItem(
				margin, margin, QSizePolicy::Expanding, QSizePolicy::Preferred));
		honorLabel = new HonorLabel(this);
		priceLabel = new ContentLabel(NORMAL_FONT, this);
		QFontMetrics qfm(QApplication::font());
		priceLabel->setFixedHeight(qfm.height() * 2);
		connect(authorLabel, SIGNAL(clicked()), SLOT(authorMoreSlot()));
		
		titleLayout->addWidget(titleLabel);
		titleLayout->addLayout(authorLayout);
		titleLayout->addItem(
			new QSpacerItem(
				margin, margin, QSizePolicy::Preferred, QSizePolicy::Preferred));
		titleLayout->addWidget(honorLabel);
		titleLayout->addItem(
			new QSpacerItem(
				margin, margin, QSizePolicy::Preferred, QSizePolicy::Preferred));
		titleLayout->addWidget(priceLabel);
		titleLayout->addItem(
			new QSpacerItem(
				margin, 2 *  margin, QSizePolicy::Preferred, QSizePolicy::Preferred));

		QHBoxLayout *opLayout = new QHBoxLayout;
		opLayout->setMargin(0);
		opLayout->setSpacing(0);
		butWidget = new EntryButtonWidget(QApplication::font(), this);
		connect(butWidget, SIGNAL(fileDownloaded(const opds::EntryView::FileInfo &)),
			SIGNAL(fileDownloaded(const opds::EntryView::FileInfo &)));
		opLayout->addWidget(butWidget);
		opLayout->addItem(
			new QSpacerItem(
				margin, margin, QSizePolicy::Preferred, QSizePolicy::Preferred));
		
		titleLayout->addLayout(opLayout);

		baseMsgLayout->addWidget(thumbnailLabel, 0, Qt::AlignTop);
		baseMsgLayout->addItem(
			new QSpacerItem(
				margin, margin, QSizePolicy::Fixed, QSizePolicy::Preferred));
		baseMsgLayout->addLayout(titleLayout);
		baseMsgLayout->addItem(
			new QSpacerItem(
				2 * margin, margin, QSizePolicy::Preferred, QSizePolicy::Preferred));
	
		layout->addLayout(baseMsgLayout);
//		layout->addItem(new QSpacerItem(margin, margin, QSizePolicy::Preferred, QSizePolicy::Preferred));
		layout->addItem(new QSpacerItem(margin, margin, QSizePolicy::Preferred, QSizePolicy::Preferred));

		QHBoxLayout *clickLabelLayout = new QHBoxLayout;

		favoriteLabel = new ClickLabel(BOLD_NORMAL, parent);
		favoriteLabel->setText(addFavoriteStr);
		
		cartLabel = new ClickLabel(BOLD_NORMAL, parent);
		cartLabel->setText(addCartStr); 
		clickLabelLayout->addWidget(favoriteLabel);
		clickLabelLayout->addItem(new QSpacerItem(margin, margin, QSizePolicy::Expanding, QSizePolicy::Preferred));
		clickLabelLayout->addWidget(cartLabel);
		clickLabelLayout->addItem(new QSpacerItem(margin, margin, QSizePolicy::Expanding, QSizePolicy::Preferred));
		
		connect(favoriteLabel, SIGNAL(clicked()), SLOT(favoriteSlot()));
		connect(cartLabel, SIGNAL(clicked()), SLOT(cartSlot()));
		
		layout->addLayout(clickLabelLayout);
		
		contentLabel = new ContentLabel(SMALL_FONT, parent);
		contentLabel->setWordWrap(true);
		//contentLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		contentLabel->setStyleSheet("a:link { color: #000000; }");
		layout->addWidget(contentLabel);

		QHBoxLayout *collapseLayout = new QHBoxLayout;
		QList<eink::PopupMenu::Item> lst;
		collapseButton = new eink::PopupMenuButton(showMoreStr, lst, -1, parent);
		collapseButton->hide();
		connect(collapseButton, SIGNAL(clicked()), SLOT(collapseSlot()));
		collapseLayout->addWidget(collapseButton);
		collapseLayout->addItem(
			new QSpacerItem(margin, margin, QSizePolicy::Expanding, QSizePolicy::Preferred));
		
		//layout->addItem(new QSpacerItem(margin, margin * 4, QSizePolicy::Preferred, QSizePolicy::Expanding));
		layout->addLayout(collapseLayout);
		catalogButton = NULL;

		commentLabel = NULL; 
		commentButton = NULL;

		layout->addItem(new QSpacerItem(20, eink::SystemManager::getInstance()->getMargin(), QSizePolicy::Preferred, QSizePolicy::Preferred));
		QVBoxLayout *alsoBuyLayout = new QVBoxLayout;
		ContentLabel *alsoBuyLabel = new ContentLabel(BOLD_LARGE, parent);
		alsoBuyLabel->setText(tr("Also busy"));
		alsoBuyDefaultPixmap = new QPixmap(ZLSApplication::qpeDir() + "pics/obookstore/entry_also_buy_thumbnail.png");
		alsoBuyView = new opds::ThumbnailView(alsoBuyDefaultPixmap, 3, parent);
		alsoBuyView->setShowPageNumber(true);		
		alsoBuyLayout->addWidget(alsoBuyLabel);
		alsoBuyLayout->addWidget(alsoBuyView);
		connect(alsoBuyView, SIGNAL(selected(const opds::Entry &)),
			SLOT(alsoBuySelected(const opds::Entry &)));

		//layout->addWidget(new eink::LineLabel(1, parent));
		layout->addLayout(alsoBuyLayout);

		QVBoxLayout *basicInfoLayout = new QVBoxLayout(parent);
		ContentLabel *basicInfoLabel = new ContentLabel(BOLD_LARGE, tr("Basic Information"), parent);
		//basicInfoLayout->addItem(new QSpacerItem(margin, margin * 2, QSizePolicy::Preferred, QSizePolicy::Preferred));		

		langLabel = new ContentLabel(NORMAL_FONT, parent);
		createdLabel = new ContentLabel(NORMAL_FONT, parent);
		publishDateLabel = new ContentLabel(NORMAL_FONT, parent);
		publisherLabel = new ContentLabel(NORMAL_FONT, parent);	
	
		basicInfoLayout->addWidget(basicInfoLabel);
		basicInfoLayout->addWidget(langLabel);
		basicInfoLayout->addWidget(publishDateLabel);
		basicInfoLayout->addWidget(createdLabel);
		basicInfoLayout->addWidget(publisherLabel);

		//layout->addWidget(new eink::LineLabel(1, parent));
		layout->addLayout(basicInfoLayout);

		//layout->addWidget(new eink::LineLabel(1, parent));		
		//layout->addItem(new QSpacerItem(margin, margin, QSizePolicy::Preferred, QSizePolicy::Expanding));
		labelList.append(titleLabel);
		labelList.append(authorLabel);
		labelList.append(priceLabel);
		labelList.append(favoriteLabel);
		labelList.append(cartLabel);		
		labelList.append(contentLabel);
		if (commentLabel)
			labelList.append(commentLabel);
		labelList.append(alsoBuyLabel);
		labelList.append(basicInfoLabel);
		labelList.append(langLabel);
		labelList.append(createdLabel);
		labelList.append(publishDateLabel);
		labelList.append(publisherLabel);
		//labelList.append(thumbnailLabel);

		changeFont();
		return widget;
	}

}
