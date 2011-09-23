#ifndef WOWDIRMANAGER_H
#define WOWDIRMANAGER_H

#include <QDir>
#include <QListWidget>
#include "ui_WoWDirManager.h"
#include "classes.h"

class QAbstractItemModel;
class QItemSelectionModel;

class WoWDirManager : public QDialog
{
	Q_OBJECT

public:
    explicit WoWDirManager(QWidget *parent = 0);
	~WoWDirManager();

	void SetupList();
	void UpdateList();

	// Icons
	QIcon iconVanilla;
	QIcon iconTBC;
	QIcon iconWotLK;
	QIcon iconCata;
	QIcon iconPTR;

	QString selected;		// The Selected WoWDir's name

	QListWidget *List;

private:
	void saveDir(st_WoWDir);		// Save the Directory

private slots:
	void on_WDM_bDirAdd_clicked();
	void on_buttonBox_Cancel_clicked() {
		QDialog::close();
	}

private:
	Ui::WoWDirManager *ui_WoWDirManager;
};

QString WoWDirGroupName(st_WoWDir);
st_WoWDir ScanWoWDir(QDir, int, int);

#endif