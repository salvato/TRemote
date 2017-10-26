#ifndef TREMOTE_H
#define TREMOTE_H

#include <QMainWindow>

namespace Ui {
class TRemote;
}

class TRemote : public QMainWindow
{
  Q_OBJECT

public:
  explicit TRemote(QWidget *parent = 0);
  ~TRemote();

private:
  Ui::TRemote *ui;
};

#endif // TREMOTE_H
