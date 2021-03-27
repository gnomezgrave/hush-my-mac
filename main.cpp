#include "silencer.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Silencer w;
    w.show();
    return a.exec();
}
