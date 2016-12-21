#include "ddv5commentmanager.h"
#include "ddv5commentrequest.h"
#include "ddv5commentparser.h"
#include "ddv5readplanview.h"
#include "dangdang/dangdangutil.h"
#include "dangdang/ddentryview.h"
#include "bookstoreutil.h"
#include "einkmediawidget.h"
#include "systemmanager.h"
#include "scrollarea.h"
#include "opdsutil.h"
#include "imageutil.h"
#include "label.h"
#include "zlsapplication.h"
#include "entrythumbnaillabel.h"
#include "view/textinputdialog.h"
#include "model/imdef.h"

#include <QtGui>
#include <QObject>
#include <QPixmap>

#define COLUMNS (3)

namespace dangdangv5 {
	CommentManager *CommentManager::instance = NULL;

	CommentManager *CommentManager::getInstance()
	{
		if (instance == NULL)
			instance = new CommentManager;
		return instance;
	}

	CommentManager::CommentManager()
	{
		request = NULL;
	}

	CommentManager::~CommentManager()
	{
	}
	
	void CommentManager::setEntry(const opds::Entry &)
	{
	}
	
	void CommentManager::addComment(const opds::Link &)
	{
	}

	opds::FeedRequest *CommentManager::getQueryBarRequest()
	{
		return new QueryBarListRequest(opds::BookStoreUtil::getInstance()->getNetworkMgr());
	}
	
	opds::FeedRequest *CommentManager::getCreatebarRequest()
	{
		return new CreateBarRequest(opds::BookStoreUtil::getInstance()->getNetworkMgr());
	}

	opds::FeedRequest *CommentManager::getCommentRequest()
	{
		if (request == NULL) {
			QNetworkAccessManager *mgr = opds::BookStoreUtil::getInstance()->getNetworkMgr();
			request = new dangdangv5::CommentRequest(mgr, NULL);
		}
		
		return request;
	}
	
	opds::FeedRequest *CommentManager::getMoreCommentRequest()
	{
		QNetworkAccessManager *mgr = opds::BookStoreUtil::getInstance()->getNetworkMgr();
		MoreCommentRequest *moreRequest = new MoreCommentRequest(mgr, NULL);
		return moreRequest;
	}
	
	//class AddCommentTask
	AddCommentTask::AddCommentTask(QObject *parent)
		:QObject(parent)
	{
		addRequest = NULL;
		addBarRequest = NULL;
	}

	AddCommentTask::~AddCommentTask()
	{
		delete addRequest;
		addRequest = NULL;
		delete addBarRequest;
		addBarRequest = NULL;
	}
	
	void AddCommentTask::addComment()
	{
		if (inputContent()) {
			if (content.isEmpty()) {
				eink::EinkMessageBox::timeoutInformation(NULL, tr("Add Comment fail"),
					tr("Comment content should not be empty!"), 3,eink::EinkMessageBox::NONE);
			} else {
				opds::Link link;
				link.userData["content"] = content;
				link.userData["title"] = "";
				CommentRequest *request = 
				dynamic_cast<CommentRequest *>(CommentManager::getInstance()->getCommentRequest());
				link.userData["barId"] = QString::number(request->getBarId());
				QNetworkAccessManager *mgr = opds::BookStoreUtil::getInstance()->getNetworkMgr();
				if (addRequest == NULL) {
					addRequest = new dangdangv5::AddCommentRequest(mgr, NULL);
					connect(addRequest, SIGNAL(finished()), this, SLOT(addSlot()));
				}
				if (addBarRequest == NULL) {
					addBarRequest = new dangdangv5::AddBarRequest(mgr, NULL);
					connect(addBarRequest, SIGNAL(finished()), this, SLOT(addBarSlot()));
				}
				addBarRequest->setBarId(request->getBarId());
				addRequest->setLink(link);
				if (request->getMemberStatus() == AddBarRequest::MEMBER) {
					addRequest->execute();
				} else {
					addBarRequest->execute();
				}
			}
		}
	}

	void AddCommentTask::addBarSlot()
	{
		if (addBarRequest->getResultCode() == AddBarRequest::SUCCESS || 
				addBarRequest->getResultCode() == AddBarRequest::HAVE_JOINED) {
			addRequest->execute();				
		} else {
			eink::EinkMessageBox::timeoutInformation(NULL, tr("Add bar fail"),
				addBarRequest->getResultMsg(), 3, eink::EinkMessageBox::NONE);
		}
	}
	
	void AddCommentTask::addSlot()
	{
		if (addRequest->getResultCode() == 0) { 
			emit addCommentFinished();
		} else {
			eink::EinkMessageBox::timeoutInformation(NULL, tr("Add comment fail"),
				addRequest->getResultMsg(), 3, eink::EinkMessageBox::NONE);
		}
	}

	bool AddCommentTask::inputContent()
	{
		QString title = QObject::tr("Add Comment");
		zlsim::TextInputDialog dlg(title);
		dlg.setTextEditType(TEXTEDIT);
			
		dlg.setSize(dlg.getDefaultSize());
		dlg.hideFinishBut();
		
		content.clear();
		if (dlg.exec() == QDialog::Accepted) {
			content = dlg.getString();
			return true;
		}
		return false;
	}


	//class CommentEntryView
	CommentEntryView::CommentEntryView(QWidget *parent)
		:QWidget(parent)
	{
		QNetworkAccessManager *mgr = opds::BookStoreUtil::getInstance()->getNetworkMgr();
		request = new CommentInfoRequest(mgr, NULL);
		connect(request, SIGNAL(finished()), SLOT(finishedSlot()));

		QVBoxLayout *layout = new QVBoxLayout(this);
		layout->setMargin(0);
		layout->setSpacing(0);
		scrollArea = new eink::ScrollArea(this);
		scrollArea->setWidget(getWidget());
		layout->addWidget(scrollArea);
	}
	
	QWidget* CommentEntryView::getWidget()
	{
		QWidget *widget = new QWidget(this);
		QWidget *parent = widget;

		const int margin = eink::SystemManager::getInstance()->getMargin();
		QVBoxLayout *layout = new QVBoxLayout;
		layout->setMargin(0);
		layout->setSpacing(margin/2);
		
		QHBoxLayout *titleLayout = new QHBoxLayout;
		titleLayout->setMargin(0);
		titleLayout->setSpacing(0);

		thumbnailLabel = new opds::EntryThumbnailLabel(parent);
		pixPath = ZLSApplication::qpeDir() + "pics/obookstore/comment_default.jpg";
		QPixmap *pixmap = new QPixmap(pixPath);
		thumbnailLabel->setPixmap(pixPath);
		thumbnailLabel->setFixedSize(pixmap->size());
		nameLabel = new dangdang::ContentLabel(dangdang::NORMAL_FONT, parent);
		dateLabel = new dangdang::ContentLabel(dangdang::NORMAL_FONT, parent);
		changeFont(nameLabel, 3);
		changeFont(dateLabel, 3);
		
		QVBoxLayout *labLayout = new QVBoxLayout;
		labLayout->setMargin(0);
		labLayout->setSpacing(0);

		labLayout->addWidget(nameLabel);
		labLayout->addWidget(dateLabel);

		titleLayout->addWidget(thumbnailLabel, 0, Qt::AlignTop);
		titleLayout->addItem(
				new QSpacerItem(margin, margin/2, QSizePolicy::Preferred, QSizePolicy::Preferred));
		titleLayout->addLayout(labLayout);
		
		contentLabel = new dangdang::ContentLabel(dangdang::NORMAL_FONT, parent);
		contentLabel->setWordWrap(true);
		changeFont(contentLabel, 2);
	
		titleLabel = new dangdang::ContentLabel(dangdang::NORMAL_FONT, parent);
		titleLabel->setWordWrap(true);
		changeFont(titleLabel, 2);
		
		layout->addItem(
				new QSpacerItem(margin/2, margin/2, QSizePolicy::Preferred, QSizePolicy::Fixed));
		layout->addLayout(titleLayout);
		layout->addWidget(titleLabel);
		layout->addWidget(contentLabel);
		layout->addStretch();

		QHBoxLayout *itemLayout = new QHBoxLayout(parent);
		itemLayout->addItem(
				new QSpacerItem(margin, margin/2, QSizePolicy::Fixed, QSizePolicy::Preferred));
		itemLayout->addLayout(layout);
		return widget;
	}

	CommentEntryView::~CommentEntryView()
	{
	}
	
	void CommentEntryView::changeFont(QLabel *label, int size)
	{
		QFont normal(QApplication::font());
		normal.setPointSize(normal.pointSize() - size);
		label->setFont(normal);
	}

	void CommentEntryView::reset()
	{
		nameLabel->clear();
		dateLabel->clear();
		thumbnailLabel->setPixmap(pixPath);
		contentLabel->clear();
		titleLabel->clear();
		scrollArea->verticalScrollBar()->triggerAction(QAbstractSlider::SliderToMinimum);
	}
	
	void CommentEntryView::needShow()
	{
		if (titleLabel->text().isEmpty()) {
			titleLabel->hide();
		} else {
			titleLabel->show();
		}
		if (dateLabel->text().isEmpty()) {
			dateLabel->hide();
		} else {
			dateLabel->show();
		}
	}
	
	void  CommentEntryView::setDefaultPixmap(const opds::Entry &e)
	{
		QString url = e.getThumnail().href;
		QString thumbnailFn = opds::ImageUtil::getInstance()->getSavedFile(url);
		if (opds::ImageUtil::getInstance()->isDownloaded(url)) {
			thumbnailLabel->setEntry(e);
		} else {
			if (thumbnailFn.trimmed().isEmpty())
				thumbnailLabel->setPixmap(pixPath);
			QPixmap pixmap(thumbnailFn);
			if (pixmap.isNull())
				thumbnailLabel->setPixmap(pixPath);
		}
	}

	void CommentEntryView::setEntry(const opds::Entry &e)
	{
		reset();
		if (eink::SystemManager::getInstance()->isDebug())
			opds::OPDSUtil::printEntry(e);
		setDefaultPixmap(e);
		contentLabel->setText(e.content);
		titleLabel->setText(e.subTitle);
		nameLabel->setText(e.title);
		if (e.published > 0) {
			QString publishDate = CommentInfoParser::getDateTime(e.published);
			dateLabel->setText(publishDate);
		}
		request->setEntry(e);
		request->execute();
	}

	void CommentEntryView::setDetailEntry(const opds::Entry &e)
	{
		if (eink::SystemManager::getInstance()->isDebug())
			opds::OPDSUtil::printEntry(e);
		if (contentLabel->text().isEmpty() || contentLabel->text() != e.content) {
			contentLabel->setText(e.content);
			contentLabel->adjustSize();
		}
		if (titleLabel->text().trimmed().isEmpty())
			titleLabel->setText(e.subTitle);
		if (nameLabel->text().isEmpty())
			nameLabel->setText(e.title);
		QString publishDate = CommentInfoParser::getDateTime(e.published);
		if (dateLabel->text().isEmpty())
			dateLabel->setText(publishDate);
		needShow();
	}

	void CommentEntryView::finishedSlot()
	{
		qDebug() << "CommentEntryView" << __func__ << "request finished !!!!";
		opds::Entry entry = request->getEntry();
		setDetailEntry(entry);
	}

	//class RecommendTagsView
	RecommendTagsView::RecommendTagsView(QWidget *parent)
		:QWidget(parent)
	{
		QVBoxLayout *layout = new QVBoxLayout;
		const int margin = eink::SystemManager::getInstance()->getMargin();
		layout->setSpacing(margin);
		layout->setMargin(margin);
		
		contentLab = new QLabel;
		tips = tr("Select tag for bar");
		loading = tr("Loading, please waiting...");
		contentLab->setText(loading);
		
		layout->addWidget(contentLab);
		layout->addItem(new QSpacerItem(0, margin, QSizePolicy::Preferred, QSizePolicy::Preferred));
		
		tagLayout = new QGridLayout;
		tagLayout->setHorizontalSpacing(margin * 2);
		request = new GetRecommendTagsRequest(opds::BookStoreUtil::getInstance()->getNetworkMgr());
		connect(request, SIGNAL(finished()), SLOT(finishedSlot()));
		layout->addLayout(tagLayout);
		layout->addStretch();

		setLayout(layout);
	}

	RecommendTagsView::~RecommendTagsView()
	{ 
		delete request;
		request = NULL;
	}	
	
	void RecommendTagsView::showEvent(QShowEvent *e)
	{
		request->execute();
		QWidget::showEvent(e);
	}
	
	void RecommendTagsView::selectTagSlot()
	{
		EinkButton *button = dynamic_cast<EinkButton *>(sender());
		QString title = button->text();
		const opds::Feed &feed = request->getFeed();	
		std::list<opds::Entry>::const_iterator it = feed.entryList.begin();
		for (; it != feed.entryList.end(); ++it) {
			if (it->title == title) {
				emit selected(*it);
				break;
			}
		}
	}

	void RecommendTagsView::finishedSlot()
	{
		const opds::Feed &feed = request->getFeed();	
		if (feed.entryList.size() == 0)
			return;
		contentLab->setText(tips);

		if (buttonList.size() == 0) {
			std::list<opds::Entry>::const_iterator it = feed.entryList.begin();
			for (; it != feed.entryList.end(); ++it) {
				EinkButton *button = new EinkButton;
				button->setText(it->title);
				buttonList.append(button);
				connect(button, SIGNAL(clicked()), SLOT(selectTagSlot()));
			}
		} else {
			std::list<opds::Entry>::const_iterator it = feed.entryList.begin();
			QList<EinkButton *>::iterator bIt = buttonList.begin();
			for (; it != feed.entryList.end() && bIt != buttonList.end();
					++it, ++bIt) {
				(*bIt)->setText(it->title);
				if ((*bIt)->isHidden())
					(*bIt)->show();	
			}

			if (it != feed.entryList.end()) {
				for (; it != feed.entryList.end(); ++it) {
					EinkButton *button = new EinkButton;
					button->setText(it->title);
					buttonList.append(button);
					connect(button, SIGNAL(clicked()), SLOT(selectTagSlot()));
				}
			}

			if (bIt != buttonList.end()) {
				for (; bIt != buttonList.end(); ++bIt) {
					(*bIt)->hide();
				}	
			}
		}
		
		drawTag();
	}
	
	void RecommendTagsView::drawTag()
	{
		for (int i = 0; i < buttonList.size(); i++) {
			int row = i / COLUMNS;
			int col = i % COLUMNS;
			tagLayout->addWidget(buttonList[i], row, col);
		}
	}
	
}

