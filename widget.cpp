#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#define cmd qDebug()

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    ui->solutionLine->setValidator(new QRegExpValidator(QRegExp("^[1-9]{1}\\d{,4}"), this));
    p= new Parser(this);

    QFile f(QDir::currentPath()+"/base.json");
    if(f.open(QFile::ReadOnly))
        base = QJsonObject(QJsonDocument::fromJson(QString(f.readAll()).toUtf8()).object());
    else
        downloadBase();


    connect(ui->findButton,&QPushButton::clicked,this,&Widget::find);
    connect(ui->listWidget,&QListWidget::currentTextChanged,this,&Widget::showSolution);

    connect(ui->removeButton, &QPushButton::clicked, this, [this]() {
        ui->textBox->setText(ui->textBox->toPlainText().replace(QRegExp("(//)[^\n]*\n"),"\n").remove(QRegExp("/\\*.*\\*/")));
    });
    connect(ui->copyButton, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(ui->textBox->toPlainText());
    });
    connect(ui->copyUrl, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(ui->urlBox->text());
    });
    connect(ui->copyPath, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(ui->pathBox->text());
    });
    connect(ui->updateBaseButton, &QPushButton::clicked, this, &Widget::downloadBase);

    connect(p,&Parser::downloadFilesProgresChanged,this,[this](double val) {
       QMetaObject::invokeMethod(ui->downloadProgress,"setText",Qt::QueuedConnection,Q_ARG(QString,QString::number(val)));
    });


    // test
    connect(p,&Parser::downloadFilesFinished,this,[this](int files, QStringList s) {
       QMetaObject::invokeMethod(ui->downloadProgress,"setText",Qt::QueuedConnection,Q_ARG(QString,""));
       cmd << ">>" << files<< "\n" <<s;
    });

    // test
    connect(ui->downloadButton, &QPushButton::clicked , this,[this]() {
        std::thread t(&Parser::downloadFiles,p,base,"C:/Users/Artem/Desktop/test",ui->solutionLine->text().toInt());
        t.detach();
    });
}

Widget::~Widget()
{
    QFile f(QDir::currentPath()+"/base.json");
    f.open(QFile::WriteOnly);
    f.write(QJsonDocument(base).toJson());
    delete ui;
}

void Widget::find()
{
    ui->listWidget->clear();
    QString key = ui->solutionLine->text();
    currentKey = key;
    if(solutions.contains(key))
    {
        for(auto e : solutions.value(key))
        {
            QListWidgetItem *i = new QListWidgetItem();
            i->setBackground(QColor(0,255,0,50));
            i->setText(e->name);
            ui->listWidget->addItem(i);
        }
        return;
    }

    QJsonArray sols = base.value(key).toArray();
    for(auto e: sols) {
        QListWidgetItem *i = new QListWidgetItem();
        i->setBackground(QColor(255,0,0,50));
        i->setText("Loading...");
        ui->listWidget->addItem(i);
        QString ref = e.toString();

        QNetworkAccessManager *manager = new QNetworkAccessManager();
        QNetworkRequest request(QUrl(ref.toUtf8()));
        manager->get(request);

        QObject::connect(manager, &QNetworkAccessManager::finished, manager, [=](QNetworkReply *reply) {
            if(currentKey != key)
                return ;
            if(reply->error() == QNetworkReply::NoError) {
                QString suffix = ref.mid(ref.lastIndexOf('.')+1);
                QByteArray data = reply->readAll();
                QString name = ref.mid(ref.lastIndexOf("/")+1);

                QString path = QDir::currentPath()+"/Downloaded_Files";
                QDir().mkdir(path);
                QFile f(path+"\\"+name);

                solutions[key].append(new Solution(name,path+"/"+name,ref,suffix));

                f.open(QFile::WriteOnly);
                f.write(data);
                f.close();

                i->setText(name);
                i->setBackground(QColor(0, 255, 0, 50));
            }
            else
                i->setText("Error");
            manager->deleteLater();
        });
    }
}

void Widget::downloadBase()
{
    if(downloading)
        return;
    downloading = true;
    connect(p,&Parser::parseLinksProgresChanged,this,[this](double val) {
        ui->downloadProgress->setText("Parse links: "+QString::number(((double)((int)(val*10)))/10)+"%");
    });

    std::thread t([&]() {
        base = p->parseAllLinks();
        QMetaObject::invokeMethod(ui->downloadProgress,"setText",Qt::QueuedConnection,Q_ARG(QString,""));
        downloading = false;
    });
    t.detach();
}

void Widget::showSolution(const QString &name)
{
    if(name.isNull())
        return;
    Solution * sol = *std::find_if(solutions.value(currentKey).begin(),solutions.value(currentKey).end(),[&](Solution *s) { return s->name == name;});
    if(!sol)
        return;
    QString s = sol->type;
    ui->urlBox->setText(sol->ref);
    ui->pathBox->setText(sol->path);
    ui->urlBox->setCursorPosition(0);
    ui->pathBox->setCursorPosition(0);

    if((QStringList() << "cpp" << "c" << "cs" << "py" << "rb" << "pas" << "java" << "js" << "").contains(s,Qt::CaseInsensitive))
    {
        QFile f(sol->path);
        cmd << sol->path;
        f.open(QFile::ReadOnly);
        ui->textBox->setText(f.readAll());

    }
    else if (QMessageBox::question(this, "", "Open "+name+"in another program?",
                                   QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
        QDesktopServices::openUrl(QUrl(sol->path));

}






