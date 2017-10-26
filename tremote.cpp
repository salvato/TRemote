#include "tremote.h"
#include "ui_tremote.h"

TRemote::TRemote(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::TRemote)
{
  ui->setupUi(this);
}

TRemote::~TRemote()
{
  delete ui;
}
