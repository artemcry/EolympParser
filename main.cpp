#include "solutionparser.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SolutionParser w;
    w.show();
    return a.exec();
}
