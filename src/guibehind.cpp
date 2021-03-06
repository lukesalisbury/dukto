/* DUKTO - A simple, fast and multi-platform file transfer tool for LAN users
 * Copyright (C) 2011 Emanuele Colombo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "guibehind.h"

#include "settings.h"
#include "miniwebserver.h"

#include "platform.h"
#include "updateschecker.h"

#include <QHash>
#include <QGuiApplication>
#include <QQmlContext>

#include <QTimer>
#include <QDesktopServices>
#include <QDir>
#include <QClipboard>
#include <QRegExp>
#include <QThread>
#include <QTemporaryFile>
#include <QFileDialog>
#include <QtGui>
#if defined(Q_WS_S60)
#define SYMBIAN
#endif

#if defined(Q_WS_SIMULATOR)
#define SYMBIAN
#endif

#ifdef SYMBIAN
#include <QNetworkConfigurationManager>
#include <QNetworkConfiguration>
#include <QMessageBox>
#endif

#define NETWORK_PORT 4644 // 6742

GuiBehind::GuiBehind(QQmlApplicationEngine *engine) :
	QObject(nullptr), mShowBackTimer(nullptr), mPeriodicHelloTimer(nullptr),
	mClipboard(nullptr), mMiniWebServer(nullptr), mSettings(nullptr), mDestBuddy(nullptr),
	mUpdatesChecker(nullptr)
{    
    // Status variables
    //mView->setGuiBehindReference(this);
    setCurrentTransferProgress(0);
    setTextSnippetSending(false);
    setShowUpdateBanner(false);

    // Clipboard object
    mClipboard = QGuiApplication::clipboard();
    connect(mClipboard, SIGNAL(dataChanged()), this, SLOT(clipboardChanged()));
    clipboardChanged();

    // Add "Me" entry
    mBuddiesList.addMeElement();

    // Add "Ip" entry
    mBuddiesList.addIpElement();

    // Settings
    mSettings = new Settings(this);

    // Mini web server
    mMiniWebServer = new MiniWebServer(NETWORK_PORT + 1);

    // Destination buddy
    mDestBuddy = new DestinationBuddy(this);

    // Change current folder
    QDir::setCurrent(mSettings->currentPath());

    // Set current theme color
    mTheme.setThemeColor(mSettings->themeColor());

    // Init buddy list

    engine->rootContext()->setContextProperty("buddiesListData", &mBuddiesList);
    engine->rootContext()->setContextProperty("recentListData", &mRecentList);
    engine->rootContext()->setContextProperty("ipAddressesData", &mIpAddresses);
    engine->rootContext()->setContextProperty("guiBehind", this);
    engine->rootContext()->setContextProperty("destinationBuddy", mDestBuddy);
    engine->rootContext()->setContextProperty("theme", &mTheme);

    // Register protocol signals
    connect(&mDuktoProtocol, SIGNAL(peerListAdded(Peer)), this, SLOT(peerListAdded(Peer)));
    connect(&mDuktoProtocol, SIGNAL(peerListRemoved(Peer)), this, SLOT(peerListRemoved(Peer)));
    connect(&mDuktoProtocol, SIGNAL(receiveFileStart(QString)), this, SLOT(receiveFileStart(QString)));
    connect(&mDuktoProtocol, SIGNAL(transferStatusUpdate(qint64,qint64)), this, SLOT(transferStatusUpdate(qint64,qint64)));
    connect(&mDuktoProtocol, SIGNAL(receiveFileComplete(QStringList*,qint64)), this, SLOT(receiveFileComplete(QStringList*,qint64)));
    connect(&mDuktoProtocol, SIGNAL(receiveTextComplete(QString*,qint64)), this, SLOT(receiveTextComplete(QString*,qint64)));
    connect(&mDuktoProtocol, SIGNAL(sendFileComplete(QStringList*)), this, SLOT(sendFileComplete(QStringList*)));
    connect(&mDuktoProtocol, SIGNAL(sendFileError(int)), this, SLOT(sendFileError(int)));
    connect(&mDuktoProtocol, SIGNAL(receiveFileCancelled()), this, SLOT(receiveFileCancelled()));
    connect(&mDuktoProtocol, SIGNAL(sendFileAborted()), this, SLOT(sendFileAborted()));

    // Register other signals
    connect(this, SIGNAL(remoteDestinationAddressChanged()), this, SLOT(remoteDestinationAddressHandler()));

    // Say "hello"
    mDuktoProtocol.setPorts(NETWORK_PORT, NETWORK_PORT);
    mDuktoProtocol.initialize();
    mDuktoProtocol.sayHello(QHostAddress::Broadcast);

    // Periodic "hello" timer
    mPeriodicHelloTimer = new QTimer(this);
    connect(mPeriodicHelloTimer, SIGNAL(timeout()), this, SLOT(periodicHello()));
    mPeriodicHelloTimer->start(60000);

    // Load GUI
    engine->load(QUrl("qrc:/qml/dukto/Dukto.qml"));

//#ifndef Q_WS_S60
//    view->restoreGeometry(mSettings->windowGeometry());
//#endif

    // Start random rotate
    mShowBackTimer = new QTimer(this);
    connect(mShowBackTimer, SIGNAL(timeout()), this, SLOT(showRandomBack()));

    mShowBackTimer->start(10000);

    // Enqueue check for updates
//    mUpdatesChecker = new UpdatesChecker();
//    connect(mUpdatesChecker, SIGNAL(updatesAvailable()), this, SLOT(showUpdatesMessage()));
//    QTimer::singleShot(2000, mUpdatesChecker, SLOT(start()));

}

GuiBehind::~GuiBehind()
{
    mDuktoProtocol.sayGoodbye();

    if (mUpdatesChecker) mUpdatesChecker->deleteLater();
    if (mMiniWebServer) mMiniWebServer->deleteLater();
    if (mShowBackTimer) mShowBackTimer->deleteLater();
    if (mPeriodicHelloTimer) mPeriodicHelloTimer->deleteLater();
    if (mDestBuddy) mDestBuddy->deleteLater();
}

// Add the new buddy to the buddy list
void GuiBehind::peerListAdded(Peer peer) {
    mBuddiesList.addBuddy(peer);
}

// Remove the buddy from the buddy list
void GuiBehind::peerListRemoved(Peer peer) {

    // Check if currently is shown the "send" page for that buddy
    if (((overlayState() == "send")
         || ((overlayState() == "showtext") && textSnippetSending()))
            && (mDestBuddy->ip() == peer.address.toString()))
        emit hideAllOverlays();

    // Check if currently is shown the "transfer complete" message box
    // for the removed user as destination
    if ((overlayState() == "message") && (messagePageBackState() == "send")
            && (mDestBuddy->ip() == peer.address.toString()))
        setMessagePageBackState("");

    // Remove from the list
    mBuddiesList.removeBuddy(peer.address.toString());
}

void GuiBehind::showRandomBack()
{
    // Look for a random element
	int i = (QRandomGenerator::global()->generateDouble() * 1.0 / RAND_MAX) * (mBuddiesList.rowCount() + 1);

    // Show back
    if (i < mBuddiesList.rowCount()) mBuddiesList.showSingleBack(i);
}

void GuiBehind::clipboardChanged()
{
    mClipboardTextAvailable = (mClipboard->text() != "");
    emit clipboardTextAvailableChanged();
}

void GuiBehind::receiveFileStart(QString senderIp)
{
    // Look for the sender in the buddy list
    QString sender = mBuddiesList.buddyNameByIp(senderIp);
    if (sender == "")
        setCurrentTransferBuddy("remote sender");
    else
        setCurrentTransferBuddy(sender);

    // Update user interface
    setCurrentTransferSending(false);

    emit transferStart();
}

void GuiBehind::transferStatusUpdate(qint64 total, qint64 partial)
{
    // Stats formatting
    if (total < 1024)
        setCurrentTransferStats(QString::number(partial) + " B of " + QString::number(total) + " B");
    else if (total < 1048576)
        setCurrentTransferStats(QString::number(partial * 1.0 / 1024, 'f', 1) + " KB of " + QString::number(total * 1.0 / 1024, 'f', 1) + " KB");
    else
        setCurrentTransferStats(QString::number(partial * 1.0 / 1048576, 'f', 1) + " MB of " + QString::number(total * 1.0 / 1048576, 'f', 1) + " MB");

    double percent = partial * 1.0 / total * 100;
    setCurrentTransferProgress(percent);


}

void GuiBehind::receiveFileComplete(QStringList *files, qint64 totalSize) {

    // Add an entry to recent activities
    QDir d(".");
    if (files->size() == 1)
        mRecentList.addRecent(files->at(0), d.absoluteFilePath(files->at(0)), "file", mCurrentTransferBuddy, totalSize);
    else
        mRecentList.addRecent("Files and folders", d.absolutePath(), "misc", mCurrentTransferBuddy, totalSize);

    // Update GUI

    //QGuiApplication::alert(mView, 5000);
    emit receiveCompleted();
}

void GuiBehind::receiveTextComplete(QString *text, qint64 totalSize)
{
    // Add an entry to recent activities
    mRecentList.addRecent("Text snippet", *text, "text", mCurrentTransferBuddy, totalSize);

    // Update GUI

    //QGuiApplication::alert(mView, 5000);
    emit receiveCompleted();
}

void GuiBehind::showTextSnippet(QString text, QString sender)
{
    setTextSnippet(text);
    setTextSnippetBuddy(sender);
    setTextSnippetSending(false);
    emit gotoTextSnippet();
}

void GuiBehind::openFile(QString path)
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void GuiBehind::openDestinationFolder()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(QDir::currentPath()));
}

void GuiBehind::changeDestinationFolder()
{
    // Show system dialog for folder selection
	QString dirname = QFileDialog::getExistingDirectory(nullptr, "Change folder", ".",
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dirname == "") return;

#ifdef SYMBIAN
    // Disable saving on C:
    if (dirname.toUpper().startsWith("C:")) {

        setMessagePageTitle("Destination");
        setMessagePageText("Receiving data on C: is disabled for security reasons. Please select another destination folder.");
        setMessagePageBackState("settings");
        emit gotoMessagePage();
        return;
    }
#endif

    // Set the new folder as current
    QDir::setCurrent(dirname);

    // Save the new setting
    setCurrentPath(dirname);
}

void GuiBehind::refreshIpList()
{
    mIpAddresses.refreshIpList();
}

void GuiBehind::showSendPage(QString ip)
{
    // Check for a buddy with the provided IP address
    QStandardItem *buddy = mBuddiesList.buddyByIp(ip);
	if (buddy == nullptr) return;

    // Update exposed data for the selected user
    mDestBuddy->fillFromItem(buddy);

    // Preventive update of destination buddy
    if (mDestBuddy->ip() == "IP")
        setCurrentTransferBuddy(remoteDestinationAddress());
    else
        setCurrentTransferBuddy(mDestBuddy->username());

    // Preventive update of text send page
    setTextSnippetBuddy(mDestBuddy->username());
    setTextSnippetSending(true);
    setTextSnippet("");

    // Show send UI
    emit gotoSendPage();
}

void GuiBehind::sendDroppedFiles(QStringList *files)
{
    if (files->count() == 0) return;

    // Check if there's no selected buddy
    // (but there must be only one buddy in the buddy list)
    if (overlayState() == "")
    {
        if (mBuddiesList.rowCount() != 3) return;
        showSendPage(mBuddiesList.fistBuddyIp());
    }

    // Send files
    QStringList toSend = *files;
    startTransfer(toSend);
}
void GuiBehind::sendFiles(QStringList files)
{
	if (overlayState() == "")
	{
		if (mBuddiesList.rowCount() != 3) return;
		showSendPage(mBuddiesList.fistBuddyIp());
	}
	startTransfer(files);
}

void GuiBehind::sendSomeFiles()
{
    // Show file selection dialog
	QStringList files = QFileDialog::getOpenFileNames(nullptr, "Send some files");
    if (files.count() == 0) return;

    // Send files
    QStringList toSend = files;
    startTransfer(toSend);
}

void GuiBehind::sendFolder()
{
    // Show folder selection dialog
	QString dirname = QFileDialog::getExistingDirectory(nullptr, "Change folder", ".",
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dirname == "") return;

    // Send files
    QStringList toSend;
    toSend.append(dirname);
    startTransfer(toSend);
}

void GuiBehind::sendClipboardText()
{
    // Get text to send
    QString text = mClipboard->text();
#ifndef Q_WS_S60
    if (text == "") return;
#else
    if (text == "") {
        setMessagePageTitle("Send");
        setMessagePageText("No text appears to be in the clipboard right now!");
        setMessagePageBackState("send");
        emit gotoMessagePage();
        return;
    }
#endif

    // Send text
    startTransfer(text);
}

void GuiBehind::sendText()
{
    // Get text to send
    QString text = textSnippet();
    if (text == "") return;

    // Send text
    startTransfer(text);
}

void GuiBehind::sendScreen()
{
    // Minimize window
    //mView->setWindowState(Qt::WindowMinimized);

    QTimer::singleShot(500, this, SLOT(sendScreenStage2()));
}

void GuiBehind::sendScreenStage2() {

    // Screenshot
    QScreen *screenid = QGuiApplication::primaryScreen();

    QPixmap screen =  screenid->grabWindow(0);

    // Restore window


    // Salvataggio screenshot in file
    QTemporaryFile tempFile;
    tempFile.setAutoRemove(false);
    tempFile.open();
    mScreenTempPath = tempFile.fileName();
    tempFile.close();
    screen.save(mScreenTempPath, "JPG", 95);

    // Prepare file transfer
    QString ip;
    qint16 port;
    if (!prepareStartTransfer(&ip, &port)) return;

    // Start screen transfer
    mDuktoProtocol.sendScreen(ip, port, mScreenTempPath);
}

void GuiBehind::startTransfer(QStringList files)
{
    // Prepare file transfer
    QString ip;
    qint16 port;
    if (!prepareStartTransfer(&ip, &port)) return;

    // Start files transfer
    mDuktoProtocol.sendFile(ip, port, files);
}

void GuiBehind::startTransfer(QString text)
{
    // Prepare file transfer
    QString ip;
    qint16 port;
    if (!prepareStartTransfer(&ip, &port)) return;

    // Start files transfer
    mDuktoProtocol.sendText(ip, port, text);
}

bool GuiBehind::prepareStartTransfer(QString *ip, qint16 *port)
{
    // Check if it's a remote file transfer
    if (mDestBuddy->ip() == "IP") {

        // Remote transfer
        QString dest = remoteDestinationAddress();

        // Check if port is specified
        if (dest.contains(":")) {

            // Port is specified or destination is malformed...
            QRegExp rx("^(.*):([0-9]+)$");
            if (rx.indexIn(dest) == -1) {

                // Malformed destination
                setMessagePageTitle("Send");
                setMessagePageText("Hey, take a look at your destination, it appears to be malformed!");
                setMessagePageBackState("send");
                emit gotoMessagePage();
                return false;
            }

            // Get IP (or hostname) and port
             QStringList capt = rx.capturedTexts();
             *ip = capt[1];
             *port = capt[2].toInt();
        }
        else {

            // Port not specified, using default
            *ip = dest;
            *port = 0;
        }
        setCurrentTransferBuddy(*ip);
    }
    else {

        // Local transfer
        *ip = mDestBuddy->ip();
        *port = mDestBuddy->port();
        setCurrentTransferBuddy(mDestBuddy->username());
    }

    // Update GUI for file transfer
    setCurrentTransferSending(true);
    setCurrentTransferStats("Connecting...");
    setCurrentTransferProgress(0);

    emit transferStart();
    return true;
}

void GuiBehind::sendFileComplete(QStringList *files)
{
    // To remove warning
    files = files;

    // Show completed message
    setMessagePageTitle("Send");
#ifndef Q_WS_S60
    setMessagePageText("Your data has been sent to your buddy!\n\nDo you want to send other files to your buddy? Just drag and drop them here!");
#else
    setMessagePageText("Your data has been sent to your buddy!");
#endif
    setMessagePageBackState("send");


    // Check for temporary file to delete
    if (mScreenTempPath != "") {

        QFile file(mScreenTempPath);
        file.remove();
        mScreenTempPath = "";
    }

    emit gotoMessagePage();
}

void GuiBehind::remoteDestinationAddressHandler()
{
    // Update GUI status
    setCurrentTransferBuddy(remoteDestinationAddress());
    setTextSnippetBuddy(remoteDestinationAddress());
}

// Returns true if the application is ready to accept
// drag and drop for files to send
bool GuiBehind::canAcceptDrop()
{
    // There must be the send page shown and,
    // if it's a remote destination, it must have an IP
    if (overlayState() == "send")
        return !((mDestBuddy->ip() == "IP") && (remoteDestinationAddress() == ""));

    // Or there could be a "send complete" or "send error" message relative to a
    // determinate buddy
    else if ((overlayState() == "message") && (messagePageBackState() == "send"))
        return true;

    // Or there could be just one buddy in the list
    else if (mBuddiesList.rowCount() == 3)
        return true;

    return false;
}

// Handles send error
void GuiBehind::sendFileError(int code)
{
    setMessagePageTitle("Error");
    setMessagePageText("Sorry, an error has occurred while sending your data...\n\nError code: " + QString::number(code));
    setMessagePageBackState("send");


    // Check for temporary file to delete
    if (mScreenTempPath != "") {

        QFile file(mScreenTempPath);
        file.remove();
        mScreenTempPath = "";
    }

    emit gotoMessagePage();
}

// Handles receive error
void GuiBehind::receiveFileCancelled()
{
    setMessagePageTitle("Error");
    setMessagePageText("An error has occurred during the transfer... The data you received could be incomplete or broken.");
    setMessagePageBackState("");

    emit gotoMessagePage();
}

// Event handler to catch the "application activate" event
bool GuiBehind::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);
    // On application activatio, I send a broadcast hello
    if (event->type() == QEvent::ApplicationActivate)
        mDuktoProtocol.sayHello(QHostAddress::Broadcast);

    return false;
}

// Changes the current theme color
void GuiBehind::changeThemeColor(QString color)
{
    mTheme.setThemeColor(color);
    mSettings->saveThemeColor(color);
}

// Called on application closing event
void GuiBehind::close()
{
    mDuktoProtocol.sayGoodbye();
}

// Reset taskbar progress status
void GuiBehind::resetProgressStatus()
{

}

// Periodic hello sending
void GuiBehind::periodicHello()
{
    mDuktoProtocol.sayHello(QHostAddress::Broadcast);
}

// Show updates message
void GuiBehind::showUpdatesMessage()
{
    setShowUpdateBanner(true);
}

// Abort current transfer while sending data
void GuiBehind::abortTransfer()
{
    mDuktoProtocol.abortCurrentTransfer();
}

// Protocol confirms that abort has been done
void GuiBehind::sendFileAborted()
{
    resetProgressStatus();
    emit gotoSendPage();
}

// ------------------------------------------------------------
// Property setter and getter

QString GuiBehind::currentTransferBuddy()
{
    return mCurrentTransferBuddy;
}

void GuiBehind::setCurrentTransferBuddy(QString buddy)
{
    if (buddy == mCurrentTransferBuddy) return;
    mCurrentTransferBuddy = buddy;
    emit currentTransferBuddyChanged();
}

int GuiBehind::currentTransferProgress()
{
    return mCurrentTransferProgress;
}

void GuiBehind::setCurrentTransferProgress(int value)
{
    if (value == mCurrentTransferProgress) return;
    mCurrentTransferProgress = value;
    emit currentTransferProgressChanged();
}

QString GuiBehind::currentTransferStats()
{
    return mCurrentTransferStats;
}

void GuiBehind::setCurrentTransferStats(QString stats)
{
    if (stats == mCurrentTransferStats) return;
    mCurrentTransferStats = stats;
    emit currentTransferStatsChanged();
}

QString GuiBehind::textSnippetBuddy()
{
    return mTextSnippetBuddy;
}

void GuiBehind::setTextSnippetBuddy(QString buddy)
{
    if (buddy == mTextSnippetBuddy) return;
    mTextSnippetBuddy = buddy;
    emit textSnippetBuddyChanged();
}

QString GuiBehind::textSnippet()
{
    return mTextSnippet;
}

void GuiBehind::setTextSnippet(QString text)
{
    if (text == mTextSnippet) return;
    mTextSnippet = text;
    emit textSnippetChanged();
}

bool GuiBehind::textSnippetSending()
{
    return mTextSnippetSending;
}

void GuiBehind::setTextSnippetSending(bool sending)
{
    if (sending == mTextSnippetSending) return;
    mTextSnippetSending = sending;
    emit textSnippetSendingChanged();
}

QString GuiBehind::currentPath()
{
    return mSettings->currentPath();
}

void GuiBehind::setCurrentPath(QString path)
{
    if (path == mSettings->currentPath()) return;
    mSettings->savePath(path);
    emit currentPathChanged();
}

bool GuiBehind::currentTransferSending()
{
    return mCurrentTransferSending;
}

void GuiBehind::setCurrentTransferSending(bool sending)
{
    if (sending == mCurrentTransferSending) return;
    mCurrentTransferSending = sending;
    emit currentTransferSendingChanged();
}

bool GuiBehind::clipboardTextAvailable()
{
    return mClipboardTextAvailable;
}

QString GuiBehind::remoteDestinationAddress()
{
    return mRemoteDestinationAddress;
}

void GuiBehind::setRemoteDestinationAddress(QString address)
{
    if (address == mRemoteDestinationAddress) return;
    mRemoteDestinationAddress = address;
    emit remoteDestinationAddressChanged();
}

QString GuiBehind::overlayState()
{
    return mOverlayState;
}

void GuiBehind::setOverlayState(QString state)
{
    if (state == mOverlayState) return;
    mOverlayState = state;
    emit overlayStateChanged();
}

QString GuiBehind::messagePageText()
{
    return mMessagePageText;
}

void GuiBehind::setMessagePageText(QString message)
{
    if (message == mMessagePageText) return;
    mMessagePageText = message;
    emit messagePageTextChanged();
}

QString GuiBehind::messagePageTitle()
{
    return mMessagePageTitle;
}

void GuiBehind::setMessagePageTitle(QString title)
{
    if (title == mMessagePageTitle) return;
    mMessagePageTitle = title;
    emit messagePageTitleChanged();
}

QString GuiBehind::messagePageBackState()
{
    return mMessagePageBackState;
}

void GuiBehind::setMessagePageBackState(QString state)
{
    if (state == mMessagePageBackState) return;
    mMessagePageBackState = state;
    emit messagePageBackStateChanged();
}

bool GuiBehind::showTermsOnStart()
{
    return mSettings->showTermsOnStart();
}

void GuiBehind::setShowTermsOnStart(bool show)
{
    mSettings->saveShowTermsOnStart(show);
    emit showTermsOnStartChanged();
}

bool GuiBehind::showUpdateBanner()
{
    return mShowUpdateBanner;
}

void GuiBehind::setShowUpdateBanner(bool show)
{
    mShowUpdateBanner = show;
    emit showUpdateBannerChanged();
}

void GuiBehind::setBuddyName(QString name)
{
    mSettings->saveBuddyName(name.replace(' ', ""));
    mDuktoProtocol.updateBuddyName();
    mBuddiesList.updateMeElement();
    emit buddyNameChanged();
}

QString GuiBehind::buddyName()
{
    return mSettings->buddyName();
}

#if defined(Q_WS_S60)
void GuiBehind::initConnection()
{
    // Connection
    QNetworkConfigurationManager manager;
    const bool canStartIAP = (manager.capabilities() & QNetworkConfigurationManager::CanStartAndStopInterfaces);
    QNetworkConfiguration cfg = manager.defaultConfiguration();
    if (!cfg.isValid() || (!canStartIAP && cfg.state() != QNetworkConfiguration::Active)) return;
    mNetworkSession = new QNetworkSession(cfg, this);
    connect(mNetworkSession, SIGNAL(opened()), this, SLOT(connectOpened()));
    connect(mNetworkSession, SIGNAL(error(QNetworkSession::SessionError)), this, SLOT(connectError(QNetworkSession::SessionError)));
    mNetworkSession->open();
}

void GuiBehind::connectOpened()
{
    mDuktoProtocol.sayHello(QHostAddress::Broadcast);
}

void GuiBehind::connectError(QNetworkSession::SessionError error)
{
    QString msg = "Unable to connecto to the network (code " + QString::number(error) + ").";
	QMessageBox::critical(nullptr, tr("Dukto"), msg);
    exit(-1);
}

#endif
