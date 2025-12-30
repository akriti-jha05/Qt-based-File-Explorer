
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileSystemModel>
#include <QListView>
#include <QLineEdit>
#include <QToolBar>
#include <QStack>
#include <QTreeWidget>
#include <QStatusBar>
#include <QSortFilterProxyModel>
#include <QTreeWidgetItem>

#include <QTimer>
class QStandardItemModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent=nullptr);

private slots:
    void navigateToPath();
    void onListDoubleClicked(const QModelIndex &index);
    void goBack();
    void refreshView();

    void createFile();
    void createFolder();
    void deleteItem();             // Normal delete (Recycle Bin)
    void deleteItemPermanent();

    void copyItem();
    void cutItem();
    void pasteItem();
    void openItem();
    void showProperties();
    void renameItem();



    void sidebarItemClicked(QTreeWidgetItem *item);
    void startSearch();
    void updateStatusBar();

private:



    QTimer *searchTimer;

    QSortFilterProxyModel *proxyModel;
    bool searchActive = false;

    QString currentPath;




    QStandardItemModel *searchModel;

    bool inSearchMode = false;

    QFileSystemModel *model;
    QListView *list;
    QLineEdit *addressBar;

    // Search
    QLineEdit *searchBar;

    // Sidebar
    QTreeWidget *sidebar;
    QStringList copiedPaths;
    bool cutMode = false;

    QStringList backHistory;
    QStringList forwardHistory;
    bool navigatingBack = false;


    QString currentDirPath() const;
    QModelIndex currentIndex() const;
    QString generateUniqueName(const QString &dirPath, const QString &name);
    bool copyDirectoryRecursively(const QString &sourcePath,
                                  const QString &destinationPath);

    void setDirectory(const QString &path);
    bool thumbnailMode = false;
    void setListViewMode();
    void setThumbnailViewMode();
    void populateSidebar();
    QString getKnownLocation(const QString &name);
    //void showSearchResults(const QStringList &results);
    void deleteItemInternal(bool permanent);
protected:
    void keyPressEvent(QKeyEvent *event) override;

};

#endif
