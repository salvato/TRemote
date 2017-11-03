#include "tremote.h"
#include <QApplication>

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  QCoreApplication::setOrganizationDomain("Gabriele.Salvato");
  QCoreApplication::setOrganizationName("Gabriele.Salvato");
  QCoreApplication::setApplicationName("TRemote");
  QCoreApplication::setApplicationVersion("1.0.0");
  TRemote w;
  w.show();

  return a.exec();
}
