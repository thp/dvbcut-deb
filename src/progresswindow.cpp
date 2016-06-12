/*  dvbcut
    Copyright (c) 2005 Sven Over <svenover@svenover.de>
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
 
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* $Id$ */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <cstdio>
#include <cstdarg>
#include <cstdlib>

#include <qprogressbar.h>
#include <qtextbrowser.h>
#include <qpushbutton.h>
#include <qapplication.h>
#include "progresswindow.h"

progresswindow::progresswindow(QWidget *parent, const char *name)
    :QDialog(parent, name, true), logoutput(),
    cancelwasclicked(false), waitingforclose(false)
  {
  ui = new Ui::progresswindowbase();
  ui->setupUi(this);
  
  Q3StyleSheetItem *item;
  item = new Q3StyleSheetItem( ui->logbrowser->styleSheet(), "h" );
  item->setFontWeight( QFont::Bold );
  item->setFontUnderline( TRUE );

  item = new Q3StyleSheetItem( ui->logbrowser->styleSheet(), "info" );

  item = new Q3StyleSheetItem( ui->logbrowser->styleSheet(), "warn" );
  item->setColor( "red" );

  item = new Q3StyleSheetItem( ui->logbrowser->styleSheet(), "error" );
  item->setColor( "red" );
  item->setFontWeight( QFont::Bold );
  item->setFontUnderline( TRUE );

  ui->cancelbutton->setPaletteBackgroundColor( QColor( 255,0,0 ) );

  show();
  qApp->processEvents();
  }

progresswindow::~progresswindow()
  {
  delete ui;
  }

void progresswindow::closeEvent(QCloseEvent *e)
  {
  if (waitingforclose)
    e->accept();
  else
    e->ignore();
  }

void progresswindow::finish()
  {
  ui->cancelbutton->setEnabled(false);
  waitingforclose=true;
  ui->cancelbutton->setText( tr( "Close" ) );
  ui->cancelbutton->setPaletteBackgroundColor( QColor( 0,255,0 ) );
  ui->cancelbutton->setEnabled(true);
  exec();
  }

void progresswindow::setprogress(int permille)
  {
  if (permille==currentprogress)
    return;
  currentprogress=permille;
  ui->progressbar->setValue(permille);
  qApp->processEvents();
  }

void progresswindow::print(const char *fmt, ...)
  {
  va_list ap;
  va_start(ap,fmt);
  char *text=0;
  if (vasprintf(&text,fmt,ap)<0 || (text==0))
    return;

  if (*text)
    ui->logbrowser->append(quotetext(text));
  else
    ui->logbrowser->append("<br>");
  free(text);
  qApp->processEvents();
  }

void progresswindow::printheading(const char *fmt, ...)
  {
  va_list ap;
  va_start(ap,fmt);
  char *text=0;
  if (vasprintf(&text,fmt,ap)<0 || (text==0))
    return;

  ui->logbrowser->append(QString("<h>")+quotetext(text)+"</h>");
  free(text);
  qApp->processEvents();
  }

void progresswindow::printinfo(const char *fmt, ...)
  {
  va_list ap;
  va_start(ap,fmt);
  char *text=0;
  if (vasprintf(&text,fmt,ap)<0 || (text==0))
    return;

  ui->logbrowser->append(QString("<info>")+quotetext(text)+"</info>");
  free(text);
  qApp->processEvents();
  }

void progresswindow::printerror(const char *fmt, ...)
  {
  va_list ap;
  va_start(ap,fmt);
  char *text=0;
  if (vasprintf(&text,fmt,ap)<0 || (text==0))
    return;

  ui->logbrowser->append(QString("<error>")+quotetext(text)+"</error>");
  free(text);
  qApp->processEvents();
  }

void progresswindow::printwarning(const char *fmt, ...)
  {
  va_list ap;
  va_start(ap,fmt);
  char *text=0;
  if (vasprintf(&text,fmt,ap)<0 || (text==0))
    return;

  ui->logbrowser->append(QString("<warn>")+quotetext(text)+"</warn>");
  free(text);
  qApp->processEvents();
  }

void progresswindow::clickedcancel()
  {
  if ((cancelwasclicked==false) && (waitingforclose==false)) {
    // button function is cancel
    cancelwasclicked=true;
    ui->cancelbutton->setEnabled(false);
    qApp->processEvents();
    ui->cancelbutton->setText( tr( "Close" ) );
    ui->cancelbutton->setPaletteBackgroundColor( QColor( 0,255,0 ) );
    ui->cancelbutton->setEnabled(true);
  } else {
    // button function is close
    close();
  }
  }

QString progresswindow::quotetext(const char *text)
  {
  return QString(text).replace('&',QString("&amp;")).replace('<',QString("&lt;")).replace('>',QString("&gt;"));
  }
