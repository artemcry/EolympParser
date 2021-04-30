#include "solutionparser.h"
#include "ui_widget.h"
#include <QDebug>
#define cmd qDebug()

SolutionParser::SolutionParser(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SolutionParser)
{
    ui->setupUi(this);
    setFixedSize(size());
    ui->solutionLine->setValidator(new QRegExpValidator(QRegExp("^[1-9]{1}\\d{,4}"), this));
    p = new Parser(this);

    QFile fbase(QDir::currentPath()+"/base.json"), fcustom(QDir::currentPath()+"/base_custom.json");
    if(fbase.open(QFile::ReadOnly))
        base = QJsonObject(QJsonDocument::fromJson(QString(fbase.readAll()).toUtf8()).object());
    else
        downloadBase();

    if(fcustom.open(QFile::ReadOnly))
        customBase = QJsonObject(QJsonDocument::fromJson(QString(fcustom.readAll()).toUtf8()).object());

    connect(ui->findButton,&QPushButton::clicked,this,&SolutionParser::find);
    connect(ui->listWidget,&QListWidget::currentRowChanged,this,&SolutionParser::showCurrentSolutionAt);
    connect(ui->addCustomButton,&QPushButton::clicked,this,&SolutionParser::addCustomSolution);

    connect(ui->removeButton, &QPushButton::clicked, this, [this]() {
        ui->textBox->setText(ui->textBox->toPlainText().replace(QRegExp("(//)[^\n]*\n"),"\n").remove(QRegExp("/\\*.*\\*/")));
    });
    connect(ui->copyButton, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(ui->textBox->toPlainText());
    });
    connect(ui->copyUrl, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(ui->urlBox->text());
    });

    connect(ui->updateBaseButton, &QPushButton::clicked, this, &SolutionParser::downloadBase);

    connect(p,&Parser::downloadFilesProgresChanged,this,[this](double val) {
        QMetaObject::invokeMethod(ui->downloadProgress,"setText",Qt::QueuedConnection,Q_ARG(QString,QString::number(val)));
    });
    connect(ui->applyButton, &QPushButton::clicked, this, [this]() {
        QJsonArray arr = customBase[currentKey].toArray();
        int index = ui->listWidget->currentIndex().row() - ui->listWidget->count() + arr.size();
        if(index < 0)
            return;
        arr[index] = ui->textBox->toPlainText();
        customBase[currentKey] = arr;
    });

    connect(p,&Parser::downloadFilesFinished,this,[this](int, QStringList) {
        QMetaObject::invokeMethod(ui->downloadProgress,"setText",Qt::QueuedConnection,Q_ARG(QString,""));
    });


    connect(ui->downloadButton, &QPushButton::clicked , this,[this]() {
        QString path = QFileDialog::getExistingDirectory(this, ("Select Output Folder"), QDir::currentPath()+"/Downloads");
        std::thread t(&Parser::downloadFiles, p, base, path, 100);
        t.detach();
    });
}

SolutionParser::~SolutionParser()
{
    QFile fbase(QDir::currentPath()+"/base.json"), fcustom(QDir::currentPath()+"/base_custom.json");
    fbase.open(QFile::WriteOnly);
    fbase.write(QJsonDocument(base).toJson());
    fcustom.open(QFile::WriteOnly);
    fcustom.write(QJsonDocument(customBase).toJson());
    QDir(tmp_files_path).removeRecursively();
    delete ui;
}

bool SolutionParser::selectSolution(QString key)
{
    ui->listWidget->clear();
    currentKey = key;

    if(!solutions.contains(key))
        return false;

    for(auto e : solutions.value(key))
    {
        QListWidgetItem *i = new QListWidgetItem();
        if(e->isCustom)
            i->setBackground(QColor(255,200,0,50));
        else if (e->isCorrect)
            i->setBackground(QColor(0,255,0,50));
        else
            i->setBackground(QColor(255,0,0,50));

        i->setText(e->name);
        ui->listWidget->addItem(i);
    }
    return true;
}

void SolutionParser::find()
{    
    QString key = ui->solutionLine->text();
    if(key == currentKey)
        return;

    if(!base.contains(key))
    {
        QMessageBox msgBox;
        msgBox.setText("Base not contains this solution");
        msgBox.setWindowTitle("Error");
        msgBox.exec();
        return;
    }
    else if (selectSolution(key))
        return;

    QJsonArray sols = base.value(key).toArray();
    for(auto e: sols) {

        solutions[key].append(new Solution(false, "Loading..."));
        Solution* so = solutions[key].last();
        solutions[key].last()->ref = e.toString();

        QListWidgetItem *item = new QListWidgetItem();
        item->setBackground(QColor(255,0,0,50));
        item->setText(so->name);
        ui->listWidget->addItem(item);

        QNetworkAccessManager *manager = new QNetworkAccessManager();
        QNetworkRequest request(QUrl(so->ref.toUtf8()));
        manager->get(request);

        QObject::connect(manager, &QNetworkAccessManager::finished, manager, [=](QNetworkReply *reply) {

            if(reply->error() == QNetworkReply::NoError) {
                so->name = so->ref.mid(so->ref.lastIndexOf("/")+1);
                so->isCorrect = true;

                QByteArray data = reply->readAll();
                QDir().mkdir(tmp_files_path);
                QFile f(so->path());
                f.open(QFile::WriteOnly);
                f.write(data);
                f.close();
            }
            else if(currentKey == key)
                item->setText("Error");

            if(currentKey == key)
                selectSolution(key);

            manager->deleteLater();
        });
    }

    for (int i = 0; i < customBase.value(key).toArray().size() ; i++) {
        QListWidgetItem *item = new QListWidgetItem();
        item->setBackground(QColor(255,200,0,50));
        solutions[key].append(new CustomSolution);
        item->setText(solutions[key].last()->name);

        ui->listWidget->addItem(item);        
    }
}

void SolutionParser::addCustomSolution()
{
    SaveSolution *s = new SaveSolution(currentKey);
    s->exec();
    if(s->Key().isEmpty())
        return;
    Parser::appendJsonArray(customBase, s->Key(), QJsonArray() << ui->textBox->toPlainText());

    solutions[s->Key()].append(new CustomSolution());
    if(currentKey == s->Key())
        selectSolution(s->Key());
}

void SolutionParser::downloadBase()
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

void SolutionParser::showCurrentSolutionAt(int index)
{
    if(index == -1)
        return;

    Solution* sol = solutions[currentKey].at(index);
    cmd << sol->path();
    ui->urlBox->setText(sol->ref);

    if(!sol->isCorrect)
        return;

    ui->urlBox->setText(sol->ref);
    ui->urlBox->setCursorPosition(0);

    if(sol->isCustom)
    {
        QJsonArray arr = customBase[currentKey].toArray();
        ui->textBox->setText(arr[index - ui->listWidget->count() + arr.size()].toString());
    }
    else if(codeLangs.contains(sol->lang(), Qt::CaseInsensitive))
    {
        QFile f(sol->path());
        if(f.open(QFile::ReadOnly))
            ui->textBox->setText(f.readAll());
        else
            QMessageBox::warning(this, "", "Unable to open file: "+sol->path());
    }
    else
        QDesktopServices::openUrl(QUrl(sol->path()));

}
