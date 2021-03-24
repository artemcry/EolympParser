#ifndef PARSER_H
#define PARSER_H

#include <QObject>
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMap>
#include <QDir>
#include <QFileInfoList>
#include <QDebug>
#include <QtNetwork/QNetworkAccessManager>
#include <QEventLoop>
#include <QtNetwork/QNetworkReply>
#include <thread>
#include <QEventLoop>
#include <QThread>
#include <math.h>

class Counter : public QObject
{
    Q_OBJECT
    int index,count;
public:
    Counter(int count): index(count),count(count) { }
    void reset(int count)
    {
        index = this->count =  count;
    }
    int getCount()
    {
        return count;
    }
    void oneCompleted()
    {
        if(!--index)
            emit allDone();
    }
signals:
    void allDone();
};


class Parser : public QObject
{
    Q_OBJECT
    QString adjustFileName(QString name);
    static void appendJsonArray(QJsonObject &toIt, const QString &key, const QJsonArray &arr);
    const int volumeCount_SiteAda = 103, volumeCount_GitHub = 9;
    int parseProgress = 0;

public:    
    explicit Parser(QObject *parent = nullptr);
    QJsonObject parseLinks_SiteAda();
    QJsonObject  parseLinks_GitHub();
    QJsonObject parseAllLinks();
    void downloadFiles(const QJsonObject &base, const QString &folder, const int thread_count);

signals:
    void parseFinished(const QString&);
    void downloadFilesProgresChanged(double val);
    void downloadFilesFinished(int files, QStringList errorFiles);
    void parseLinksProgresChanged(double val);
};

#endif // PARSER_H
