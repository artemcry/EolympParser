#include "parser.h"
#define cmd qDebug()

QString Parser::adjustFileName(QString name)
{
    return name.replace(QRegExp("["+QRegExp::escape("\\/:*?\"<>|" ) + "]"), QString("_"));;
}

Parser::Parser(QObject *parent) : QObject(parent) { }

void Parser::appendJsonArray(QJsonObject &obj,const QString &key, const QJsonArray &arr)
{
    QJsonArray o = arr;
    if(obj.contains(key))
        for(auto e : obj.value(key).toArray())
            o << e;
    obj[key] = o;
}

QJsonObject Parser::parseLinks_SiteAda()
{
    QJsonObject res;
    Counter threadCounter(volumeCount_SiteAda);
    QEventLoop loop;
    QObject::connect(&threadCounter, &Counter::allDone, &loop, &QEventLoop::quit );

    for(int i = 1; i <= volumeCount_SiteAda; i++) {
        QNetworkAccessManager *manager = new QNetworkAccessManager(&loop);
        QString volumeRef = "https://site.ada.edu.az/~medv/acm/Docs%20e-olimp/Volume%20"+QString::number(i)+"/";
        QNetworkRequest request(QUrl(volumeRef.toUtf8()));
        manager->get(request);

        connect(manager,&QNetworkAccessManager::finished,this,[&,volumeRef] (QNetworkReply *reply) {
            QString html = reply->readAll();
            QRegExp rx("<a href=.{0,20}\">.{0,20}</a> ");
            QStringList fnames;
            int pos = 0;
            while ((pos = rx.indexIn(html, pos)) != -1) {
                fnames << rx.cap(0);
                pos += rx.matchedLength();
            }

            for(QString fileName : fnames)
            {
                if(fileName.contains("files"))  // folders with files html page skip
                    continue;

                fileName.remove(0,9);
                fileName = fileName.section("\">",1,1);
                fileName.remove(fileName.size()-5,5);

                QString key;
                for(auto e: fileName) {
                    if(!e.isDigit())
                        break;
                    key += e;
                }
                appendJsonArray(res, key, QJsonArray() << volumeRef+fileName);
            }
            emit parseLinksProgresChanged((double)parseProgress++/(volumeCount_GitHub+volumeCount_SiteAda)*100.0);
            threadCounter.oneCompleted();
        });
    }
    loop.exec();  // wait until all requests are answered
    return  res;
}

QJsonObject Parser::parseLinks_GitHub()
{        
    QJsonObject res;
    Counter counter(volumeCount_GitHub);
    QEventLoop loop;
    QObject::connect(&counter, &Counter::allDone, &loop, &QEventLoop::quit);

    for (int i = 0; i < volumeCount_GitHub; i++)
    {
        QNetworkAccessManager *manager = new QNetworkAccessManager(&loop);        
        QNetworkRequest request(QUrl("https://github.com/memo735/e-olymp/tree/master/"+QString::number(i)+"000-"+QString::number(i)+"999"));
        manager->get(request);
        QObject::connect(manager, &QNetworkAccessManager::finished, &loop, [&,i] (QNetworkReply *reply) {
            QString html = reply->readAll();
            QRegExp rx("<a class=\"js-navigation-open Link--primary\" title=\".{1,200}\" data-pjax=\"#repo-content-pjax-container\" href=\"");
            QStringList fnames;
            int pos = 0;
            while ((pos = rx.indexIn(html, pos)) != -1) {
                fnames << rx.cap(0);
                pos += rx.matchedLength();
            }

            for(QString fileName : fnames)
            {
                fileName.remove(0,51);
                fileName.remove(fileName.size()-49,49);
                QString key;
                if(fileName[0] == "P")
                    key = QString::number(fileName.mid(7,4).toInt());
                else
                    key = QString::number(fileName.mid(0,4).toInt());
                fileName.insert(0,"https://raw.githubusercontent.com/memo735/e-olymp/master/"+QString::number(i)+"000-"+QString::number(i)+"999/");
                appendJsonArray(res, key, QJsonArray() << fileName);
            }
            emit parseLinksProgresChanged((double)parseProgress++/(volumeCount_GitHub+volumeCount_SiteAda)*100.0);
            counter.oneCompleted();
        });
    }
    loop.exec();
    return  res;
}

QJsonObject Parser::parseAllLinks()
{
    parseProgress = 0;
    QJsonObject sada,git;
    sada = parseLinks_SiteAda();
    git =  parseLinks_GitHub();
    for(QString key: git.keys())
        appendJsonArray(sada, key, git.value(key).toArray());
    return sada;
}

void Parser::downloadFiles(const QJsonObject &base, const QString &folder,const int thread_count = 100)
{
    int file_count = 0;
    for (auto e : base)
        file_count += e.toArray().size();

    int i = 0, progrs = 1;
    QEventLoop loop;
    QStringList errorFiles;
    Counter threadCounter(file_count%thread_count);

    for(auto name: base.keys())
    {
        for(QJsonValue ref : base.value(name).toArray())
        {
            i++;
            QNetworkAccessManager *manager = new QNetworkAccessManager();            
            QNetworkRequest request(QUrl(ref.toString()));
            manager->get(request);

            QObject::connect(manager, &QNetworkAccessManager::finished,manager,[&,i,manager,ref](QNetworkReply *r) {
                QString fileName = ref.toString();;
                std::reverse(fileName.begin(),fileName.end());
                fileName = fileName.section("/",0,0);
                std::reverse(fileName.begin(),fileName.end());

                QFile f(folder+"/"+adjustFileName(fileName));
                if(f.open(QIODevice::WriteOnly))
                    f.write(r->readAll());
                else
                    errorFiles << fileName;

                manager->deleteLater();
                threadCounter.oneCompleted();
                emit downloadFilesProgresChanged((double)progrs++/file_count*100.0);
                //cmd << "DOWNLOADED:" << i << " " << f.fileName();
            });
            //cmd << "REQUIREST: " << i;
            if(i == threadCounter.getCount())
            {
                i = 0;
                QEventLoop l;
                connect(&threadCounter, &Counter::allDone, &l, &QEventLoop::quit);
                l.exec();  // Waiting for the specified number of requests to be processed
                threadCounter.reset(thread_count);
            }
        }        
    }
    emit downloadFilesFinished(file_count, errorFiles);
}
