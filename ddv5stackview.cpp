#include "ddv5stackview.h"
#include "dangdang/dangdangutil.h"
#include "bookstoreutil.h"
#include "bookstorefactory.h"
#include "baseitemview.h"
#include "ddv5commentmanager.h"
#include "ddv5contentrequest.h"
#include "systemmanager.h"
#include "scrollarea.h"
#include "label.h"
#include "opdsutil.h"
#include "baseitemmodel.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDesktopWidget>
#include <QDate>
#include <QDateTime>
#include <QDebug>
#include <QApplication>
#include <QFont>
#include <QPainter>
#include <QVector>
#include "backtextbutton.h"

#define SEPERATOR (" :  ")

namespace dangdangv5 {

	//class CommentListView
	CommentListView::CommentListView(QNetworkAccessManager *m, QWidget *parent)
		:opds::FeedView(m, parent)
	{
		setTitle(tr("CommentList"));
		manager = dangdangv5::CommentManager::getInstance();
		setShowInfo(false);
	}

	CommentListView::~CommentListView()
	{
	}
	
	void CommentListView::update(TSubject *s)
	{
		opds::FeedView::update(s);
		QList<eink::ModelIndex> data = getModel()->getCurrentPage();
		if (!data.isEmpty()) {
			int index = getModel()->getCurrentPage().last().index() + 1;
			emit currentIndex(index);
		}
	}

	void CommentListView::setRow(int r)
	{
		getModel()->setRow(r);
	}

	void CommentListView::loadEntry(const opds::Entry &e)
	{
		if (e.id.isEmpty() || !e.isValid())
			return;
		qDebug() << "CommentListView" << __func__ << __FILE__ << "entry id " << e.id;
		getFeedRequest()->reset();
		getFeedRequest()->setEntry(e);
		loadData();
		total = 0;
	}

	opds::FeedRequest *CommentListView::createFeedRequest()
	{
		return manager->getMoreCommentRequest();
	}

	bool CommentListView::isEmpty()
	{
		return getFeedRequest()->getFeed().entryList.empty();
	}
	
	int CommentListView::getTotalNum()
	{
		int tmp = getFeedRequest()->getFeed().totalResults;
		if (tmp > total)
			total = tmp;
		return total;
	}

	void CommentListView::currentSelected(int idx)
	{
		qDebug() << "CommentListView" << __FILE__ << __func__ << "index" << idx;
		eink::ModelIndex mi = getModel()->getIndex(idx);
		const opds::Feed &feed = getFeedRequest()->getFeed();
		const opds::Entry &entry = feed.getEntryById(mi.value("id"));

		if (eink::SystemManager::getInstance()->isDebug()) {
			opds::OPDSUtil::printEntry(entry);
			qDebug()  << "CommentListView" << __func__;
		}
		
		if (entry.isValid()) {
			emit openEntry(entry);			
		} else {
			qWarning() << "CommentListView" << __func__ << "ERROR: the entry not vaild";
			opds::OPDSUtil::printEntry(entry);
		}
	}


	//class ContentListView
	ContentListView::ContentListView(QNetworkAccessManager *m, QWidget *parent)
		:opds::FeedView(m, parent)
	{
		setTitle(tr("ContentList"));
	}

	ContentListView::~ContentListView()
	{
	}

	void ContentListView::update(TSubject *s)
	{
		opds::FeedView::update(s);
		QList<eink::ModelIndex> data = getModel()->getCurrentPage();
		if (!data.isEmpty()) {
			int index = getModel()->getCurrentPage().last().index() + 1;
			emit currentIndex(index);
		}
	}

	void ContentListView::loadEntry(const opds::Entry &e)
	{
		qDebug() << "ContentListView" << __func__ << __FILE__ << "entry id " << e.id;
		getFeedRequest()->setEntry(e);
		loadData();
	}
	
	int ContentListView::getResultCode()
	{
		return getFeedRequest()->getResultCode();
	}

	opds::FeedRequest *ContentListView::createFeedRequest()
	{
		return new dangdangv5::ContentRequest(opds::BookStoreUtil::getInstance()->getNetworkMgr());
	}

	bool ContentListView::isEmpty()
	{
		return getFeedRequest()->getFeed().entryList.empty();
	}
	
	int ContentListView::getTotalNum()
	{
		return getFeedRequest()->getFeed().totalResults;
	}
	
	void ContentListView::currentSelected(int idx)
	{
		qDebug() << "ContentListView" << __FILE__ << __func__ << "index" << idx;
	}

	//class CopyRightView
	CopyRightView::CopyRightView(QWidget *parent)
		:QWidget(parent)
	{
		QVBoxLayout *layout = new QVBoxLayout;
		const int margin = eink::SystemManager::getInstance()->getMargin();
		layout->setSpacing(margin/2);
		
		titleLab = new QLabel(this);
		authorLab = new QLabel(this);
		isbnLab = new QLabel(this);
		publisherLab = new QLabel(this);
		publishDateLab = new QLabel(this);
		fileSizeLab = new QLabel(this);
		wordCntLab = new QLabel(this);
		priceLab = new QLabel(this);
		
		layout->addItem(new QSpacerItem(margin, margin, QSizePolicy::Preferred, QSizePolicy::Fixed));
		layout->addWidget(titleLab);
		layout->addWidget(authorLab);
		layout->addWidget(isbnLab);
		layout->addWidget(publisherLab);
		layout->addWidget(publishDateLab);
		layout->addWidget(fileSizeLab);
		layout->addWidget(wordCntLab);
		layout->addWidget(priceLab);
		layout->addStretch();
	
		QHBoxLayout *hLayout = new QHBoxLayout(this);
		hLayout->setSpacing(0);
		hLayout->setMargin(0);
		hLayout->addItem(new QSpacerItem(margin*3, margin, QSizePolicy::Preferred, QSizePolicy::Preferred));
		hLayout->addLayout(layout);

		labList.append(titleLab);
		labList.append(authorLab);
		labList.append(isbnLab);
		labList.append(publisherLab);
		labList.append(publishDateLab);
		labList.append(fileSizeLab);
		labList.append(wordCntLab);
		labList.append(priceLab);
		changeFont();
	}

	CopyRightView::~CopyRightView()
	{
	}

	void CopyRightView::changeFont()
	{
		QFont normal(QApplication::font());
		normal.setPointSize(normal.pointSize() - 2);
		QList<QLabel *>::iterator it = labList.begin();
		for (; it != labList.end(); it++) {
			(*it)->setFont(normal);
		}
	}
	
	void CopyRightView::reset()
	{
		QList<QLabel *>::iterator it = labList.begin();
		for (; it != labList.end(); it++) {
			(*it)->clear();
		}
	}
	
	void CopyRightView::needShow()
	{
		QList<QLabel *>::iterator it = labList.begin();
		for (; it != labList.end(); it++) {
			if (!(*it)->text().isEmpty()) {
				(*it)->show();
			} else {
				(*it)->hide();
			}
		}
	}

	void CopyRightView::loadEntry(const opds::Entry &entry)
	{
		reset();
		opds::Link link = entry.getLink(opds::REL_ALTERNATE);
		if (!entry.title.isEmpty()) {
			QString title = tr("Title") + SEPERATOR + entry.title;
			titleLab->setText(title);
			titleLab->setWordWrap(true);
		}
		
		QStringList authors;
		QString authorsText;
		std::list<opds::Author>::const_iterator it = entry.authors.begin();
		for (; it != entry.authors.end(); ++it) 
			authors.append(it->name);
		if (!authors.isEmpty()) {
			authorsText = tr("Author") + SEPERATOR + authors.join(",");
			authorLab->setText(authorsText);
			authorLab->setWordWrap(true);
		}

		QDate date = QDateTime::fromTime_t(entry.published).date();
		QString publishDate;
		if (date.year() > 1900 && entry.published > 0) {
			publishDate = QString::number(date.year()) + "-" 
				+ QString::number(date.month()) + "-" + QString::number(date.day());
			publishDate.prepend(SEPERATOR);
			publishDate.prepend(tr("PublishDate"));
			publishDateLab->setText(publishDate);
		}

		if (!link.userData.value("isbn").isEmpty()) {
			QString isbn = tr("ISBN") + SEPERATOR + link.userData.value("isbn");
			isbnLab->setText(isbn);
		}

		if (!entry.dcPublisher.isEmpty()) {
			QString publisher = tr("Publisher") + SEPERATOR + entry.dcPublisher;
			publisherLab->setText(publisher);
		}
		
		if (!link.userData.value("fileSize").isEmpty()) {
			QString fileSize;
			double size = link.userData.value("fileSize").toDouble();
			if (size > 1024) {
				size = size / 1024;
				fileSize = tr("FileSize") + SEPERATOR + QString::number(size, 'f', 2) + tr("MB");
			} else {
				fileSize = tr("FileSize") + SEPERATOR + link.userData.value("fileSize") + tr("KB");
			}
			fileSizeLab->setText(fileSize);
		}

		if (!link.userData.value("wordCnt").isEmpty()) {
			QString wordCnt = tr("WordCnt") + SEPERATOR + link.userData.value("wordCnt") + tr("Million words");
			wordCntLab->setText(wordCnt);
		}

		if (entry.id != entry.subTitle && !entry.subTitle.isEmpty()) {
			QString price = tr("Price")	+ SEPERATOR + tr("5 bell/thousand");
			priceLab->setText(price);
		}
		needShow();
	}

	//class SummaryView
	SummaryView::SummaryView(QWidget *parent)
		:QWidget(parent)
	{
		QVBoxLayout *layout = new QVBoxLayout(this);
		const int margin = eink::SystemManager::getInstance()->getMargin();
		layout->setMargin(margin);
		summLab = new QLabel;
		summLab->setAlignment(Qt::AlignTop | Qt::AlignLeft);
		summLab->setWordWrap(true);
		QFont font = summLab->font();
		font.setPointSize(font.pointSize() - 2);
		summLab->setFont(font);

		scrollArea = new eink::ScrollArea(this);
		scrollArea->setWidget(summLab);
		layout->addWidget(scrollArea);
	}

	SummaryView::~SummaryView()
	{
	}

	void SummaryView::loadContent(const QString &val)
	{
		scrollArea->verticalScrollBar()->triggerAction(QAbstractSlider::SliderToMinimum);
		summLab->clear();
		summLab->setText(val);
		summLab->adjustSize();
	}

	//class ClickLabel
	ClickLabel::ClickLabel(dangdang::FontType ft, QWidget *parent)
		:QLabel(parent)
		,pressed(false)
		,alignment(Qt::AlignCenter)
		,fontType(ft)
	{
		hasSel = false;
	}


	ClickLabel::~ClickLabel()
	{
	}

	dangdang::FontType ClickLabel::getFontType()
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
	
	void ClickLabel::setSelect(bool select)
	{
		hasSel = select;
		update();	
	}

	void ClickLabel::paintEvent(QPaintEvent *)
	{
		QPainter p(this);
		QPen pen(Qt::black);
		QFontMetrics fm(font());
		QFont normal = font();
		normal.setWeight(QFont::Normal);
		QLine line;
		
		QRect r(rect());
		if (pressed) {
			p.fillRect(rect(), Qt::black);
			pen.setColor(Qt::white);
		} else {
			p.fillRect(rect(), Qt::white);
		}
		
		if (hasSel) {
			normal.setWeight(QFont::Bold);
		} else {
			normal.setWeight(QFont::Light);
		}

		p.setPen(pen);
		p.drawText(r, alignment, text());
		if (!isEnabled()) {
			normal.setWeight(QFont::Light);
		}
		setFont(normal);
	}

	//class ContentLabel
	ContentLabel::ContentLabel(dangdang::FontType t, QWidget *parent)
		:QLabel(parent)
		,fontType(t)
		,pressed(false)
	{
	}
	
	ContentLabel::ContentLabel(const QString &txt, QWidget *parent)
		:QLabel(txt, parent)
		,pressed(false)
	{
		setAlignment(Qt::AlignCenter);
	}
	
	ContentLabel::~ContentLabel()
	{
	}

	void ContentLabel::mousePressEvent(QMouseEvent *e)
	{
		pressed = true;
		update();
		QWidget::mousePressEvent(e);
	}

	void ContentLabel::mouseReleaseEvent(QMouseEvent *e)
	{
		if (pressed && isEnabled()) {
			emit clicked();
		}
		pressed = false;
		update();
		QWidget::mouseReleaseEvent(e);		
	}

	dangdang::FontType ContentLabel::getFontType()
	{
		return fontType;
	}
	
	//class PictureLabel
	PictureLabel::PictureLabel(int type, QWidget *parent)
		:QWidget(parent)
	{
		pressed = false;
		const int margin = eink::SystemManager::getInstance()->getMargin();
		QBoxLayout *layout = new QHBoxLayout(this);
		layout->setMargin(0);
		layout->setSpacing(10);

		pixLab = new QLabel;
		contentLab = new QLabel;
		QFont font;
		font.setPointSize(font.pointSize()-2);
		font.setWeight(QFont::Normal);
		contentLab->setFont(font);
			
		widget = new QWidget;
		widget->setFixedWidth(margin);
		if (0 == type) {
			layout->addWidget(widget);
			layout->addWidget(pixLab);
			layout->addWidget(contentLab);
		} else {
			layout->addWidget(contentLab);
			layout->addWidget(pixLab);
		}
		layout->addStretch();
	}
	
	PictureLabel::~PictureLabel()
	{
	}
		
	void PictureLabel::setPixmap(const QString &pixPath)
	{
		QPixmap pixmap = QPixmap(pixPath);
		pixLab->setPixmap(pixmap);
	}

	void PictureLabel::setText(const QString &content)
	{
		contentLab->setText(content);
	}
	
	const QString PictureLabel::text()
	{
		return contentLab->text();
	}

	void PictureLabel::mousePressEvent(QMouseEvent *e)
	{
		const QPixmap *pixmap = pixLab->pixmap();
		QImage image = pixmap->toImage();
		image.invertPixels();
		pixLab->setPixmap(QPixmap::fromImage(image));
		contentLab->setStyleSheet("QLabel{ color: white; };");
		pressed = true;
		update();
		QWidget::mousePressEvent(e);
	}

	void PictureLabel::mouseReleaseEvent(QMouseEvent *e)
	{
		if (pressed) {
			const QPixmap *pixmap = pixLab->pixmap();
			QImage image = pixmap->toImage();
			image.invertPixels();
			pixLab->setPixmap(QPixmap::fromImage(image));
			contentLab->setStyleSheet("QLabel{ color: black; };");
			pressed = false;
			update();
			emit clicked();
		}
		QWidget::mouseReleaseEvent(e);
	}

	void PictureLabel::paintEvent(QPaintEvent *)
	{
		QPainter p(this);
		QPen pen(Qt::black);
		p.setPen(pen);
		
		QRect r(rect());
		if (pressed) {
			p.fillRect(rect(), Qt::black);
		} else {
			p.fillRect(rect(), Qt::white);
		}
	}

	void PictureLabel::setEnabled(bool enable)
	{
		QWidget::setEnabled(enable);
		if (!enable) {
			contentLab->setStyleSheet("QLabel{ color: gray; };");
		} else {
			contentLab->setStyleSheet("QLabel{ color: black; };");
		}
	}
	
	//class ButtonArrayLabel
	ButtonArrayLabel::ButtonArrayLabel(const QStringList &list, QWidget *parent, bool hasBackKey)
		:QWidget(parent)
	{
		QVBoxLayout *layout = new QVBoxLayout(this);
		layout->setMargin(0);
		layout->setSpacing(0);
		QHBoxLayout *hLayout = new QHBoxLayout(this);
		hLayout->setMargin(0);
		hLayout->setSpacing(0);
		
		const int border = eink::SystemManager::getInstance()->buttonBorderWidth();
		if (hasBackKey) {//eink::SystemManager::getInstance()->useVirtualBackKey()) {
			backBut = new eink::BackTextButton(this);
			backBut->setEmitEscapeKey(true);
			hLayout->addWidget(backBut);
		} else {
			backBut = NULL;
		}

		for (int i=0; i<list.size() - 1; ++i) {
			ClickLabel *clickLabel = new ClickLabel(dangdang::NORMAL_FONT);
			clickLabel->setText(list.at(i));
			hLayout->addWidget(clickLabel);
			hLayout->addWidget(new eink::LineLabel(border, Qt::Vertical));
			connect(clickLabel, SIGNAL(clicked()), SLOT(clickedSlot()));
		}
		ClickLabel *clickLabel = new ClickLabel(dangdang::NORMAL_FONT);
		clickLabel->setText(list.at(list.size()-1));
		hLayout->addWidget(clickLabel);
		connect(clickLabel, SIGNAL(clicked()), SLOT(clickedSlot()));
	
		//layout->addWidget(new eink::LineLabel(3));
		layout->addItem(new QSpacerItem(10, 5, QSizePolicy::Preferred, QSizePolicy::Fixed));
		layout->addLayout(hLayout);
		layout->addItem(new QSpacerItem(10, 10, QSizePolicy::Preferred, QSizePolicy::Fixed));
		buttonList = list;
		reset();
	}

	ButtonArrayLabel::~ButtonArrayLabel()
	{
	}

	void ButtonArrayLabel::setHasBackKey(bool b)
	{
		if (!backBut)
			return;
		if (!b) {
			backBut->hide();
			backBut->setEnabled(false);
		} else {
			backBut->setEnabled(true);
			backBut->show();
		}
	}
	
	void ButtonArrayLabel::reset()
	{
		button = buttonList.first();
		update();
	}
	
	void ButtonArrayLabel::selected(const QString &text)
	{
		button = text;
		update();
	}

	void ButtonArrayLabel::clickedSlot()
	{
		ClickLabel *label = dynamic_cast<ClickLabel *>(sender());
		button = label->text();
		emit currentSelected(button);
		update();	
	}

	void ButtonArrayLabel::paintEvent(QPaintEvent *) 
	{
		QPainter p(this);
		QPen pen(Qt::black);
		pen.setWidth(2);
		p.setPen(pen);
		paint(button, p);
	}
	
	void ButtonArrayLabel::paint(const QString &button, QPainter &painter)
	{
		QVector<QLine> lines;
		int totalWidth = width();
		int baseHeight = height() - 2;

		int size = buttonList.size();
		int pos = buttonList.lastIndexOf(button) + 1;
		if (backBut && backBut->isEnabled()) {
			size++;
			pos++;
		}
		int peakWidht = ((totalWidth/(size*2))*(2*pos -1));
		QLine startLine(QPoint(0, baseHeight), QPoint(peakWidht-5, baseHeight));
		QLine leftLine(QPoint(peakWidht-5, baseHeight), QPoint(peakWidht, baseHeight-5));
		QLine rightLine(QPoint(peakWidht, baseHeight-5), QPoint(peakWidht+5, baseHeight));
		QLine endLine(QPoint(peakWidht+5, baseHeight), QPoint(totalWidth, baseHeight));
		lines.append(startLine);
		lines.append(leftLine);
		lines.append(rightLine);
		lines.append(endLine);
		painter.drawLines(lines);
	}
	
	QSize ButtonArrayLabel::sizeHint() const
	{
		const int margin = eink::SystemManager::getInstance()->getMargin();
		QFontMetrics fm(QApplication::font());
		return QSize(qApp->desktop()->width(), fm.height() + margin*2);
	}

}


