#include "propertiesdialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QFileInfo>
#include <QDir>

PropertiesDialog::PropertiesDialog(const QString &path, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Properties");

    QFileInfo info(path);

    QString text;
    text += "<b>Name:</b> " + info.fileName() + "<br>";
    text += "<b>Path:</b> " + path + "<br>";
    text += "<b>Type:</b> " + (info.isDir() ? "Folder" : info.suffix()) + "<br>";
    text += "<b>Size:</b> " + QString::number(info.size()) + " bytes<br>";
    text += "<b>Created:</b> " + info.birthTime().toString() + "<br>";
    text += "<b>Modified:</b> " + info.lastModified().toString() + "<br>";
    text += "<b>Hidden:</b> " + QString(info.isHidden() ? "Yes" : "No") + "<br>";
    text += "<b>Readable:</b> " + QString(info.isReadable() ? "Yes" : "No") + "<br>";
    text += "<b>Writable:</b> " + QString(info.isWritable() ? "Yes" : "No") + "<br>";

    if (info.isDir()) {
        QDir dir(path);
        QFileInfoList list = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
        text += "<b>Contains:</b> " + QString::number(list.size()) + " items<br>";
    }

    QLabel *label = new QLabel(text);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(label);
    setLayout(layout);
}
