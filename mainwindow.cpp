#include "mainwindow.h"
#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif
#include "propertiesdialog.h"
#include <QStandardItemModel>
#include <QStyledItemDelegate>

#include <QPainter>
#include <QStyleOptionViewItem>
#include <QFileIconProvider>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QMenu>
#include <QAction>
#include <QDirIterator>
#include <QTreeWidgetItem>
#include <QPushButton>
#include <QInputDialog>
#include <QStandardPaths>
#include <utility>   // for std::as_const
#include <QSortFilterProxyModel>
#include <QtConcurrent>

#include <QKeyEvent>

class HighlightDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        painter->save();

        QString text = index.data(Qt::DisplayRole).toString();
        QString key  = index.data(Qt::UserRole + 1).toString();

        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);
        opt.text.clear();

        opt.widget->style()->drawControl(
            QStyle::CE_ItemViewItem, &opt, painter);

        QRect r = option.rect.adjusted(32, 0, 0, 0); // after icon

        int pos = key.isEmpty()
                      ? -1
                      : text.toLower().indexOf(key.toLower());

        if (pos >= 0) {
            QFontMetrics fm(option.font);

            painter->drawText(r, Qt::AlignVCenter,
                              text.left(pos));

            painter->setPen(Qt::red);
            painter->drawText(
                r.adjusted(fm.horizontalAdvance(text.left(pos)), 0, 0, 0),
                Qt::AlignVCenter,
                text.mid(pos, key.length()));

            painter->setPen(Qt::black);
            painter->drawText(
                r.adjusted(fm.horizontalAdvance(
                               text.left(pos + key.length())), 0, 0, 0),
                Qt::AlignVCenter,
                text.mid(pos + key.length()));
        } else {
            painter->drawText(r, Qt::AlignVCenter, text);
        }

        painter->restore();
    }
};



static QStringList selectedPaths(QListView *view,
                                 QFileSystemModel *model,
                                 QSortFilterProxyModel *proxy)
{
    QStringList paths;
    QModelIndexList indexes = view->selectionModel()->selectedIndexes();

    QSet<QString> unique;
    for (const QModelIndex &proxyIdx : std::as_const(indexes)) {
        QModelIndex srcIdx = proxy->mapToSource(proxyIdx);
        unique.insert(model->filePath(srcIdx));
    }
    return unique.values();
}




MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{


    setWindowTitle("File Explorer");

    // 1️⃣ File system model (FIRST)
    model = new QFileSystemModel(this);
    model->setRootPath(QDir::homePath());
    model->setFilter(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files);
    model->setNameFilterDisables(true);  // default = no filtering

    list = new QListView(this);
    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterKeyColumn(0); // Name column

    list->setModel(proxyModel);

    list->setSelectionMode(QAbstractItemView::ExtendedSelection);

    currentPath = QDir::homePath();
    QModelIndex src = model->index(currentPath);
    QModelIndex proxy = proxyModel->mapFromSource(src);
    list->setRootIndex(proxy);

    list->setItemDelegate(new HighlightDelegate(list));

    //searchModel = new QStringListModel(this);
    searchModel = new QStandardItemModel(this);

    // 2️⃣ Proxy model (SECOND)


    // Double click
    connect(list, &QListView::doubleClicked,
            this, &MainWindow::onListDoubleClicked);


currentPath = QDir::homePath();
    //------------------------------
    // Sidebar (Quick Access)
    //------------------------------
    sidebar = new QTreeWidget(this);
    sidebar->setHeaderHidden(true);
    sidebar->setMaximumWidth(220);
    populateSidebar();
    connect(sidebar, &QTreeWidget::itemClicked, this, &MainWindow::sidebarItemClicked);



    //------------------------------
    // Address Bar
    //------------------------------
    addressBar = new QLineEdit(this);
    addressBar->setText(QDir::homePath());
    connect(addressBar, &QLineEdit::returnPressed, this, &MainWindow::navigateToPath);


    //------------------------------
    // Search bar
    //------------------------------
    searchBar = new QLineEdit(this);
    searchBar->setPlaceholderText("Search ");
    searchTimer = new QTimer(this);
    searchTimer->setSingleShot(true);

    connect(searchBar, &QLineEdit::textChanged,
            this, [=]() {
                searchTimer->start(350); // wait 30 ms after typing stops
            });




    connect(searchTimer, &QTimer::timeout,
            this, &MainWindow::startSearch);


    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLayout->addWidget(searchBar);


    //------------------------------
    // Toolbar
    //------------------------------
    QToolBar *toolbar = addToolBar("Main Toolbar");
    QAction *backAct = toolbar->addAction("Back");
    QAction *refreshAct = toolbar->addAction("Refresh");
    QAction *newFileAct = toolbar->addAction("New File");
    QAction *newFolderAct = toolbar->addAction("New Folder");
    QAction *deleteAct = toolbar->addAction("Delete");
    connect(deleteAct, &QAction::triggered,
            this, &MainWindow::deleteItem);

    QAction *propAct = toolbar->addAction("Properties");
    QAction *renameAct = toolbar->addAction("Rename");
    QAction *viewAct = toolbar->addAction("Toggle View");
    connect(viewAct, &QAction::triggered, this, [=](){
        if (thumbnailMode)
            setListViewMode();
        else
            setThumbnailViewMode();
    });

    // keyboards sequence
    // ===============================


    // DELETE
    QAction *delAct = new QAction(this);
    delAct->setShortcut(QKeySequence::Delete);
    delAct->setShortcutContext(Qt::ApplicationShortcut);
    addAction(delAct);
    connect(delAct, &QAction::triggered,
            this, &MainWindow::deleteItem);

    // COPY
    QAction *copyAct = new QAction(this);
    copyAct->setShortcut(QKeySequence::Copy);
    copyAct->setShortcutContext(Qt::ApplicationShortcut);
    addAction(copyAct);
    connect(copyAct, &QAction::triggered, this, &MainWindow::copyItem);

    // CUT
    QAction *cutAct = new QAction(this);
    cutAct->setShortcut(QKeySequence::Cut);
    cutAct->setShortcutContext(Qt::ApplicationShortcut);
    addAction(cutAct);
    connect(cutAct, &QAction::triggered, this, &MainWindow::cutItem);

    // PASTE
    QAction *pasteAct = new QAction(this);
    pasteAct->setShortcut(QKeySequence::Paste);
    pasteAct->setShortcutContext(Qt::ApplicationShortcut);
    addAction(pasteAct);
    connect(pasteAct, &QAction::triggered, this, &MainWindow::pasteItem);

    // NEW FILE (Ctrl + N)
    QAction *newFileShortcut = new QAction(this);
    newFileShortcut->setShortcut(QKeySequence("Ctrl+N"));
    newFileShortcut->setShortcutContext(Qt::ApplicationShortcut);
    addAction(newFileShortcut);
    connect(newFileShortcut, &QAction::triggered, this, &MainWindow::createFile);

    // REFRESH (F5)
    QAction *refreshShortcut = new QAction(this);
    refreshShortcut->setShortcut(QKeySequence::Refresh);
    refreshShortcut->setShortcutContext(Qt::ApplicationShortcut);
    addAction(refreshShortcut);
    connect(refreshShortcut, &QAction::triggered, this, &MainWindow::refreshView);



    connect(renameAct, &QAction::triggered, this, &MainWindow::renameItem);


    connect(backAct, &QAction::triggered, this, &MainWindow::goBack);
    connect(refreshAct, &QAction::triggered, this, &MainWindow::refreshView);
    connect(newFileAct, &QAction::triggered, this, &MainWindow::createFile);
    connect(newFolderAct, &QAction::triggered, this, &MainWindow::createFolder);
    connect(deleteAct, &QAction::triggered, this, &MainWindow::deleteItem);
    connect(propAct, &QAction::triggered, this, &MainWindow::showProperties);

    //------------------------------
    // Layout
    //------------------------------
    QWidget *central = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(central);

    QWidget *rightContainer = new QWidget(this);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightContainer);

    rightLayout->addWidget(addressBar);
    rightLayout->addLayout(searchLayout);
    rightLayout->addWidget(list);

    mainLayout->addWidget(sidebar);
    mainLayout->addWidget(rightContainer);
    setCentralWidget(central);

    //------------------------------
    // Status Bar
    //------------------------------
    statusBar()->showMessage("Ready");
    updateStatusBar();

    //------------------------------
    // Context menu
    //------------------------------
    list->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(list, &QListView::customContextMenuRequested, this, [=](QPoint pos){
        QModelIndex idx = list->indexAt(pos);
        QMenu menu;

        // Right-click on EMPTY AREA
        if (!idx.isValid()) {
            menu.addAction("New File", this, &MainWindow::createFile);
            menu.addAction("New Folder", this, &MainWindow::createFolder);
            menu.addSeparator();
            menu.addAction("Paste", this, &MainWindow::pasteItem);
            menu.addAction("Refresh", this, &MainWindow::refreshView);
            menu.exec(list->viewport()->mapToGlobal(pos));
            return;
        }

        // Right-click on FILE/FOLDER
        list->setCurrentIndex(idx);

        menu.addAction("Open", this, &MainWindow::openItem);
        menu.addAction("Rename", this, &MainWindow::renameItem);
        menu.addSeparator();
        menu.addAction("Copy", this, &MainWindow::copyItem);
        menu.addAction("Cut", this, &MainWindow::cutItem);
        menu.addAction("Paste", this, &MainWindow::pasteItem);
        menu.addSeparator();
        menu.addAction("Delete", this, &MainWindow::deleteItem);

        menu.addAction("Properties", this, &MainWindow::showProperties);



        menu.exec(list->viewport()->mapToGlobal(pos));
    });


}

//-------------------------------------------
// Sidebar
//-------------------------------------------
void MainWindow::populateSidebar()
{
    QStringList quickList = {
        "Home", "Desktop", "Documents", "Downloads",
        "Pictures", "Music", "Videos"
    };

    for (const QString &name : quickList) {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, name);
        sidebar->addTopLevelItem(item);
    }
}



QString MainWindow::getKnownLocation(const QString &name)
{
    if (name == "Home")
        return QDir::homePath();

    if (name == "Desktop")
        return QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);

    if (name == "Documents")
        return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

    if (name == "Downloads")
        return QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);

    if (name == "Pictures")
        return QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);

    if (name == "Music")
        return QStandardPaths::writableLocation(QStandardPaths::MusicLocation);

    if (name == "Videos")
        return QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);

    return QDir::homePath();
}


void MainWindow::sidebarItemClicked(QTreeWidgetItem *item)
{
    QString loc = getKnownLocation(item->text(0));
    if (QDir(loc).exists()) {
        setDirectory(loc);
    }
}

//-------------------------------------------
// Navigation core
//-------------------------------------------
QString MainWindow::currentDirPath() const
{
    return currentPath;
}



QModelIndex MainWindow::currentIndex() const
{
    return list->currentIndex();
}


void MainWindow::setDirectory(const QString &path)
{
    if (path == currentPath)
        return;

    // Only push history if user navigated normally
    if (!navigatingBack) {
        backHistory.append(currentPath);
        forwardHistory.clear();
    }

    navigatingBack = false;

    currentPath = path;

    QModelIndex srcIndex = model->index(path);
    QModelIndex proxyIndex = proxyModel->mapFromSource(srcIndex);
    list->setRootIndex(proxyIndex);

    addressBar->setText(path);
    startSearch();
    updateStatusBar();
}


QString MainWindow::generateUniqueName(const QString &dirPath, const QString &fileName)
{
    QFileInfo info(fileName);
    QString base = info.completeBaseName();
    QString ext  = info.suffix();

    QString newName = base + "_copy";
    if (!ext.isEmpty())
        newName += "." + ext;

    int counter = 1;
    QString fullPath = dirPath + "/" + newName;

    while (QFile::exists(fullPath)) {
        newName = base + "_copy_" + QString::number(counter++);
        if (!ext.isEmpty())
            newName += "." + ext;
        fullPath = dirPath + "/" + newName;
    }

    return newName;
}
bool MainWindow::copyDirectoryRecursively(const QString &sourcePath,
                                          const QString &destinationPath)
{
    QDir sourceDir(sourcePath);
    if (!sourceDir.exists())
        return false;

    QDir destDir(destinationPath);
    if (!destDir.exists()) {
        if (!destDir.mkpath("."))
            return false;
    }

    QFileInfoList entries = sourceDir.entryInfoList(
        QDir::NoDotAndDotDot | QDir::AllEntries);

    for (const QFileInfo &entry : std::as_const(entries)) {
        QString srcPath = entry.absoluteFilePath();
        QString destPath = destinationPath + "/" + entry.fileName();

        if (entry.isDir()) {
            if (!copyDirectoryRecursively(srcPath, destPath))
                return false;
        } else {
            if (QFile::exists(destPath))
                QFile::remove(destPath);   // safety overwrite

            if (!QFile::copy(srcPath, destPath))
                return false;
        }
    }
    return true;
}

void MainWindow::navigateToPath()
{
    QString path = addressBar->text().trimmed();
    if (!QDir(path).exists()) {
        QMessageBox::warning(this, "Error", "Directory does not exist.");
        return;
    }
    setDirectory(path);
}

void MainWindow::onListDoubleClicked(const QModelIndex &index)
{
    if (inSearchMode) {
        QString path = index.data(Qt::UserRole).toString();
        QFileInfo info(path);

        if (info.isDir())
            setDirectory(path);
        else
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));

        searchBar->clear();
        return;
    }


    QModelIndex srcIndex = proxyModel->mapToSource(index);
    QFileInfo info = model->fileInfo(srcIndex);

    if (info.isDir())
        setDirectory(info.absoluteFilePath());
    else
        openItem();
}



void MainWindow::goBack()
{
    if (backHistory.isEmpty())
        return;

    navigatingBack = true;

    QString prevPath = backHistory.takeLast();
    forwardHistory.append(currentPath);

    setDirectory(prevPath);
}



void MainWindow::refreshView()
{
    QModelIndex srcIndex = model->index(currentDirPath());
    QModelIndex proxyIndex = proxyModel->mapFromSource(srcIndex);
    list->setRootIndex(proxyIndex);

    startSearch();
    updateStatusBar();
}




//-------------------------------------------
// File operations
//-------------------------------------------
void MainWindow::createFile()
{
    QString dir = currentDirPath();
    QString name = QInputDialog::getText(this, "New File", "Enter file name:");
    if (name.isEmpty())
        return;

    QString full = dir + "/" + name;
    if (QFileInfo::exists(full)) {
        QMessageBox::warning(this, "Error",
                             "A file or folder with this name already exists.");
        return;
    }

    QFile file(full);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Error",
                             "Failed to create file.\nCheck permissions.");
        return;
    }

    file.close();
    refreshView();
}


void MainWindow::createFolder()
{
    QString dir = currentDirPath();
    QString name = QInputDialog::getText(this, "New Folder", "Enter folder name:");
    if (name.isEmpty()) return;

    QString full = dir + "/" + name;
    if (QFileInfo::exists(full)) {
        QMessageBox::warning(this, "Error", "A file or folder with this name already exists.");
        return;
    }

    QDir(dir).mkdir(name);
    refreshView();
}
void MainWindow::renameItem()
{
    QModelIndex idx = currentIndex();
    if (!idx.isValid())
        return;

    QModelIndex srcIdx = proxyModel->mapToSource(idx);
    QString oldPath = model->filePath(srcIdx);

    QFileInfo info(oldPath);
    QString oldName = info.fileName();

    // Ask user for new name
    QString newName = QInputDialog::getText(
        this,
        "Rename Item",
        "Enter new name:",
        QLineEdit::Normal,
        oldName
        );

    if (newName.isEmpty() || newName == oldName)
        return;


    if (info.isFile()) {
        QString oldExt = info.suffix();
        QString newExt = QFileInfo(newName).suffix();

        if (!oldExt.isEmpty() && oldExt != newExt) {
            if (QMessageBox::warning(
                    this,
                    "File Extension Change",
                    "Changing file extension may make the file unusable.\nDo you want to continue?",
                    QMessageBox::Yes | QMessageBox::Cancel
                    ) != QMessageBox::Yes)
                return;
        }
    }


    QString newPath = currentDirPath() + "/" + newName;

    // Check if name already exists
    if (QFileInfo::exists(newPath)) {
        QMessageBox::warning(this, "Error", "An item with this name already exists.");
        return;
    }



    // Folder rename
    bool success = false;

    if (info.isDir()) {
        QDir parent(info.absolutePath());
        success = parent.rename(oldName, newName);
    } else {
        success = QFile::rename(oldPath, newPath);
    }

    if (!success) {
        QMessageBox::warning(this, "Error", "Unable to rename item.");
        return;
    }

    refreshView();

}






void MainWindow::copyItem()
{
    if (inSearchMode) {
        QMessageBox::information(this, "Search Mode",
                                 "Exit search to perform this action.");
        return;
    }

    copiedPaths = selectedPaths(list, model, proxyModel);
    cutMode = false;
}


void MainWindow::cutItem()
{
    if (inSearchMode) {
        QMessageBox::information(this, "Search Mode",
                                 "Exit search to perform this action.");
        return;
    }

    copiedPaths = selectedPaths(list, model, proxyModel);
    cutMode = true;
}


void MainWindow::pasteItem()
{
    if (copiedPaths.isEmpty())
        return;

    QString destDir = currentDirPath();

    for (const QString &srcPath : std::as_const(copiedPaths)) {

        QFileInfo srcInfo(srcPath);
        QString destPath = destDir + "/" + srcInfo.fileName();

        //  If pasting into SAME directory, auto-rename
        if (QFile::exists(destPath)) {
            QString newName = generateUniqueName(destDir, srcInfo.fileName());
            destPath = destDir + "/" + newName;
        }

        bool success = false;

        if (srcInfo.isDir()) {
            success = copyDirectoryRecursively(srcPath, destPath);
        } else {
            success = QFile::copy(srcPath, destPath);
        }

        if (!success) {
            QMessageBox::warning(
                this,
                "Paste Failed",
                "Unable to paste: " + srcInfo.fileName()
                );
        }
    }

    //  Cut → remove originals
    if (cutMode) {
        for (const QString &srcPath : std::as_const(copiedPaths)) {
            QFileInfo info(srcPath);
            if (info.isDir())
                QDir(srcPath).removeRecursively();
            else
                QFile::remove(srcPath);
        }
        cutMode = false;
        copiedPaths.clear();
    }

    refreshView();
}





void MainWindow::openItem()
{
    QModelIndex idx = currentIndex();
    if (!idx.isValid())
        return;

    QModelIndex srcIdx = proxyModel->mapToSource(idx);
    QString path = model->filePath(srcIdx);

    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

//-------------------------------------------
//  Properties Dialog
//-------------------------------------------
void MainWindow::showProperties()
{
    QModelIndex srcIdx = proxyModel->mapToSource(currentIndex());
    QString path = model->filePath(srcIdx);

    PropertiesDialog dlg(path, this);
    dlg.exec();
}

//-------------------------------------------
// Status bar
//-------------------------------------------
void MainWindow::updateStatusBar()
{
    QDir dir(currentDirPath());
    QFileInfoList listItems = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files);

    int fileCount = 0, folderCount = 0;
    for (auto &i : listItems) {
        if (i.isDir()) folderCount++; else fileCount++;
    }

    statusBar()->showMessage(
        QString("%1 items — %2 folders, %3 files — %4")
            .arg(listItems.size())
            .arg(folderCount)
            .arg(fileCount)
            .arg(currentDirPath())
        );
}

//-------------------------------------------
// Recursive Search
//-------------------------------------------
void MainWindow::startSearch()
{
    QString text = searchBar->text().trimmed();

    // Exit search mode
    if (text.isEmpty()) {
        if (inSearchMode) {
            list->setModel(proxyModel);

            QModelIndex src = model->index(currentDirPath());
            QModelIndex proxy = proxyModel->mapFromSource(src);
            list->setRootIndex(proxy);

            inSearchMode = false;
        }
        return;
    }

    inSearchMode = true;
    list->setEnabled(false);
    statusBar()->showMessage("Searching...");

    QtConcurrent::run([=]() {

        QList<QFileInfo> matches;

        QDirIterator it(
            currentDirPath(),
            QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files,
            QDirIterator::Subdirectories
            );

        while (it.hasNext()) {
            it.next();
            QFileInfo info = it.fileInfo();

            if (info.fileName().contains(text, Qt::CaseInsensitive)) {
                matches.append(info);
            }
        }

        QMetaObject::invokeMethod(this, [=]() {

            searchModel->clear();
            QFileIconProvider iconProvider;

            for (const QFileInfo &info : matches) {
                QStandardItem *item =
                    new QStandardItem(
                        iconProvider.icon(info),   // ✅ icon
                        info.fileName()             // ✅ name only
                        );

                item->setData(info.absoluteFilePath(), Qt::UserRole); // hidden path
                item->setData(text, Qt::UserRole + 1);                // for highlight

                searchModel->appendRow(item);
            }

            list->setModel(searchModel);
            list->setRootIndex(QModelIndex());
            list->setEnabled(true);

            statusBar()->showMessage(
                QString("Found %1 item(s)").arg(matches.size())
                );

        }, Qt::QueuedConnection);
    });
}








//Toggle Vire
void MainWindow::setListViewMode()
{
    thumbnailMode = false;

    list->setViewMode(QListView::ListMode);
    list->setIconSize(QSize(32, 32));
    list->setGridSize(QSize());   // reset
    list->setSpacing(2);

    statusBar()->showMessage("Switched to List View");
}

void MainWindow::setThumbnailViewMode()
{
    thumbnailMode = true;

    // Thumbnail mode
    list->setViewMode(QListView::IconMode);
    list->setIconSize(QSize(96, 96));   // Thumbnail size
    list->setGridSize(QSize(120, 120)); // Grid space for items
    list->setSpacing(10);

    statusBar()->showMessage("Switched to Thumbnail View");
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete) {
        if (event->modifiers() & Qt::ShiftModifier)
            deleteItemPermanent();
        else
            deleteItem();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

//delete operation

void MainWindow::deleteItemInternal(bool permanent)
{
    if (inSearchMode) {
        QMessageBox::information(this, "Search Mode",
                                 "Exit search to perform this action.");
        return;
    }

    QStringList paths = selectedPaths(list, model, proxyModel);
    if (paths.isEmpty())
        return;

    QString msg = permanent
                      ? "Permanently delete selected items?\n(This cannot be undone)"
                      : "Delete selected items?\n(Moved to Recycle Bin)";

    if (QMessageBox::question(this, "Delete", msg) != QMessageBox::Yes)
        return;

#ifdef Q_OS_WIN
    for (const QString &path : std::as_const(paths)) {
        std::wstring wpath = path.toStdWString();
        wpath.push_back(L'\0');

        SHFILEOPSTRUCTW op = {};
        op.wFunc = FO_DELETE;
        op.pFrom = wpath.c_str();
        op.fFlags = permanent
                        ? FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT
                        : FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;

        SHFileOperationW(&op);
    }
#endif

    refreshView();
}
void MainWindow::deleteItem()
{
    deleteItemInternal(false);
}

void MainWindow::deleteItemPermanent()
{
    deleteItemInternal(true);
}


