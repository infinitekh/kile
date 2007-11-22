/***************************************************************************
    begin                : Sun Aug 3 2003
    copyright            : (C) 2003 by Jeroen Wijnhout
                         : (C) 2007  by Holger Danielsson
    email                : Jeroen.Wijnhout@kdemail.net
                           holger.danielsson@versanet.de
***************************************************************************/
 
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// 2007-02-15 (dani)
//  - cosmetic changes
//  - use of groupboxes to prepare further extensions

// 2007-03-12 (dani)
//  - use KileDocument::Extensions
//  - allowed extensions are always defined as list, f.e.: .tex .ltx .latex 

#include "kileprojectdlgs.h"

#include <qlabel.h>
#include <q3whatsthis.h>
#include <qfileinfo.h>
#include <q3ptrlist.h>
#include <qregexp.h>
#include <qvalidator.h>
//Added by qt3to4:
#include <Q3GridLayout>
#include <Q3VBoxLayout>

#include <klocale.h>
#include <kmessagebox.h>
#include <kurlcompletion.h>
#include <kfiledialog.h>
#include <kcombobox.h>
#include "kiledebug.h"
#include <kapplication.h>
#include <kiconloader.h>
#include <kglobal.h>

#include "kileproject.h"
#include "kiletoolmanager.h"
#include "kiledocumentinfo.h"
#include "kileconfig.h"
#include "kileextensions.h"
#include "templates.h"

const QString whatsthisName = i18n("Insert a short descriptive name of your project here.");
const QString whatsthisPath = i18n("Insert the path to your project file here. If this file does not yet exists, it will be created. The filename should have the extension: .kilepr. You can also use the browse button to insert a filename.");
const QString whatsthisExt = i18n("Insert a list (separated by spaces) of file extensions which should be treated also as files of the corresponding type in this project.");
const QString whatsthisMaster = i18n("Select the default master document. Leave empty for auto detection.");

const QString tool_default = i18n("(use global setting)");

KileProjectDlgBase::KileProjectDlgBase(const QString &caption, KileDocument::Extensions *extensions, QWidget *parent, const char * name)
	: KDialogBase( KDialogBase::Plain, caption, (Ok | Cancel), Ok, parent, name, true, true),
	m_extmanager(extensions), m_project(0)
{
	// properties groupbox
	m_pgroup = new Q3VGroupBox(i18n("Project"), plainPage());
	m_pgroup->setColumnLayout(0, Qt::Vertical );
	m_pgroup->layout()->setSpacing( 6 );
	m_pgroup->layout()->setMargin( 11 );
	m_pgrid = new Q3GridLayout( m_pgroup->layout() );
	m_pgrid->setAlignment( Qt::AlignTop );

	m_title = new KLineEdit(m_pgroup, "le_projectname");
	Q3WhatsThis::add(m_title, whatsthisName);
	m_plabel = new QLabel(i18n("Project &title:"), m_pgroup);
	m_plabel->setBuddy(m_title);
	Q3WhatsThis::add(m_plabel, whatsthisName);
	
	// extensions groupbox
	m_egroup= new Q3VGroupBox(i18n("Extensions"), plainPage());
	m_egroup->setColumnLayout(0, Qt::Vertical );
	m_egroup->layout()->setSpacing( 6 );
	m_egroup->layout()->setMargin( 11 );
	m_egrid = new Q3GridLayout( m_egroup->layout() );
	m_egrid->setAlignment( Qt::AlignTop );

	m_extensions = new KLineEdit(m_egroup, "le_ext");
	QRegExp reg("[\\. a-zA-Z0-9]+");
	QRegExpValidator *extValidator = new QRegExpValidator(reg,m_egroup);
	m_extensions->setValidator(extValidator);

	m_sel_extensions = new KComboBox( false, m_egroup );
	m_sel_extensions->setObjectName( "le_sel_ext" );
	m_sel_extensions->insertItem(i18n("Source Files"));
	m_sel_extensions->insertItem(i18n("Package Files"));
	m_sel_extensions->insertItem(i18n("Image Files"));
	m_lbPredefinedExtensions = new QLabel(i18n("Predefined:"),m_egroup);
	m_lbStandardExtensions = new QLabel(QString::null,m_egroup);

	Q3WhatsThis::add(m_sel_extensions, whatsthisExt);
	Q3WhatsThis::add(m_extensions, whatsthisExt);

	fillProjectDefaults();

	connect(m_sel_extensions, SIGNAL(highlighted(int)),
		this, SLOT(slotExtensionsHighlighted(int)));

	connect(m_extensions, SIGNAL(textChanged(const QString&)),
		this, SLOT(slotExtensionsTextChanged(const QString&)));
}

KileProjectDlgBase::~KileProjectDlgBase()
{
}

void KileProjectDlgBase::slotExtensionsHighlighted(int index)
{
	disconnect(m_extensions, SIGNAL(textChanged(const QString&)),
		this, SLOT(slotExtensionsTextChanged(const QString&)));
	m_extensions->setText(m_val_extensions[index]);
	connect(m_extensions, SIGNAL(textChanged(const QString&)),
		this, SLOT(slotExtensionsTextChanged(const QString&)));

	m_lbStandardExtensions->setText( m_val_standardExtensions[index] );
}

void KileProjectDlgBase::slotExtensionsTextChanged(const QString &text)
{
	m_val_extensions[m_sel_extensions->currentItem()] = text;
}

bool KileProjectDlgBase::acceptUserExtensions()
{
	QRegExp reg("\\.\\w+");

	for (int i=KileProjectItem::Source; i<KileProjectItem::Other; ++i) 
	{
		m_val_extensions[i-1] = m_val_extensions[i-1].trimmed();
		if ( ! m_val_extensions[i-1].isEmpty() )
		{
			// some tiny extension checks
			QStringList::ConstIterator it;
			QStringList list = QStringList::split(" ", m_val_extensions[i-1]);
			for ( it=list.begin(); it != list.end(); ++it ) 
			{
				if ( ! reg.exactMatch(*it) )
				{
					KMessageBox::error(this, i18n("Error in extension") + " '" + (*it) + "':\n" + i18n("All user defined extensions should look like '.xyz'"), i18n("Invalid extension"));
					return false;
				}
			}
		}
	}

	return true;
}

void KileProjectDlgBase::setExtensions(KileProjectItem::Type type, const QString & ext)
{
	if (m_sel_extensions->currentItem() == type-1)
		m_extensions->setText(ext);
	else
		m_val_extensions[type-1] = ext;
}

void KileProjectDlgBase::setProject(KileProject *project, bool override)
{
	m_project = project;

	if ((!override) || (project == 0))
		return;

	for (int i = KileProjectItem::Source; i < KileProjectItem::Other; ++i) 
	{
		m_val_extensions[i - 1] = project->extensions((KileProjectItem::Type) i);
	}

	m_title->setText(m_project->name());
	m_extensions->setText(m_val_extensions[0]);
	m_lbStandardExtensions->setText( m_val_standardExtensions[0] );
}

KileProject* KileProjectDlgBase::project()
{
	return m_project;
}

void KileProjectDlgBase::fillProjectDefaults()
{
	m_val_extensions[0] = QString::null;
	m_val_extensions[1] = QString::null;
	m_val_extensions[2] = QString::null;
	//m_val_extensions[3] = OTHER_EXTENSIONS;

	m_val_standardExtensions[0] = m_extmanager->latexDocuments();
	m_val_standardExtensions[1] = m_extmanager->latexPackages();
	m_val_standardExtensions[2] = m_extmanager->images();

	m_extensions->setText(m_val_extensions[0]);
	m_lbStandardExtensions->setText( m_val_standardExtensions[0] );
}


/*
 * KileNewProjectDlg
 */
KileNewProjectDlg::KileNewProjectDlg(KileTemplate::Manager *templateManager, KileDocument::Extensions *extensions, QWidget* parent, const char* name)
        : KileProjectDlgBase(i18n("Create New Project"), extensions, parent, name), m_templateManager(templateManager),
		m_filename(QString::null)
{
	// Layout
	Q3VBoxLayout *vbox = new Q3VBoxLayout(plainPage(), 6,6 );

	// first groupbox
	m_pgrid->addWidget(m_plabel, 0,0);
	m_pgrid->addWidget(m_title, 0,2);
	connect(m_title, SIGNAL(textChanged(const QString&)), this, SLOT(makeProjectPath()));

	m_location = new KLineEdit(m_pgroup, "le_projectlocation");
	m_location->setMinimumWidth(200);

	QLabel *lb1 = new QLabel(i18n("Project &file:"), m_pgroup);
	Q3WhatsThis::add(lb1, whatsthisPath);
	Q3WhatsThis::add(m_location, whatsthisPath);
	lb1->setBuddy(m_location);
	m_pbChooseDir= new KPushButton(i18n("Select Folder..."), m_pgroup, "dirchooser_button" );
	m_pbChooseDir->setPixmap( SmallIcon("fileopen") );
	int wpixmap = m_pbChooseDir->pixmap()->width();
	m_pbChooseDir->setFixedWidth(wpixmap+10);
	m_pbChooseDir->setFixedHeight(wpixmap+10);

	m_pgrid->addWidget(lb1,1,0);
	m_pgrid->addMultiCellWidget(m_location,1,1,2,3);
	m_pgrid->addWidget(m_pbChooseDir,1,5);
	m_pgrid->setColSpacing(1,8);
	m_pgrid->setColSpacing(4,8);
	m_pgrid->setColStretch(3,1);

	connect(m_pbChooseDir, SIGNAL(clicked()), this, SLOT(browseLocation()));

	// second groupbox
	Q3VGroupBox* group2= new Q3VGroupBox(i18n("File"), plainPage());
	QWidget *widget2 = new QWidget(group2);
	Q3GridLayout *grid2 = new Q3GridLayout(widget2, 3,2, 6,6);
	m_cb = new QCheckBox(i18n("Create a new file and add it to this project"),widget2);
	m_cb->setChecked(true);
	m_lb  = new QLabel(i18n("File&name (relative to where the project file is):"),widget2);
	m_file = new KLineEdit(widget2);
	m_lb->setBuddy(m_file);
	m_templateIconView = new TemplateIconView(widget2);
	m_templateIconView->setTemplateManager(m_templateManager);
	m_templateManager->scanForTemplates();
	m_templateIconView->fillWithTemplates(KileDocument::LaTeX);
	Q3WhatsThis::add(m_cb, i18n("If you want Kile to create a new file and add it to the project, then check this option and select a template from the list that will appear below."));

	grid2->addMultiCellWidget(m_cb, 0,0, 0,1);
	grid2->addWidget(m_lb, 1,0);
	grid2->addWidget(m_file, 1,1);
	grid2->addMultiCellWidget(m_templateIconView, 2,2, 0,1);
	grid2->setColStretch(1,1);
	connect(m_cb, SIGNAL(clicked()), this, SLOT(clickedCreateNewFileCb()));

	// third groupbox
	m_egrid->addWidget(m_sel_extensions, 6,0);
	m_egrid->addMultiCellWidget(m_extensions, 6,6, 1,3);
	m_egrid->addWidget(m_lbPredefinedExtensions, 7,0);
	m_egrid->addMultiCellWidget(m_lbStandardExtensions, 7,7, 1,3);

	// add to layout
	vbox->addWidget(m_pgroup);
	vbox->addWidget(group2);
	vbox->addWidget(m_egroup);
	vbox->addStretch();
	
	fillProjectDefaults();
}

KileNewProjectDlg::~KileNewProjectDlg()
{}

KileProject* KileNewProjectDlg::project()
{
	if (!m_project) {
		m_project = new KileProject(projectTitle(), KUrl(location()), m_extmanager);

		KileProjectItem::Type type;
		for (int i = KileProjectItem::Source; i < KileProjectItem::Other; ++i) {
			type = (KileProjectItem::Type) i;
			m_project->setExtensions(type, extensions(type));
		}

		m_project->buildProjectTree();
	}

	return m_project;
}

void KileNewProjectDlg::clickedCreateNewFileCb()
{
	if (m_cb->isChecked())
	{
		m_file->show();
		m_lb->show();
		m_templateIconView->show();
	}
	else
	{
		m_file->hide();
		m_lb->hide();
		m_templateIconView->hide();
	}
}

QString KileNewProjectDlg::bare()
{
	return projectTitle().toLower().trimmed().replace(QRegExp("\\s*"),"")+".kilepr";
}

void KileNewProjectDlg::browseLocation()
{
	QString dir = m_dir;
	if (!QFileInfo(m_dir).exists())
		dir = QString::null;

	dir = KFileDialog::getExistingDirectory(dir, this);

	if (! dir.isNull())
	{
		m_dir = dir;
		if (m_dir.right(1) != "/") m_dir = m_dir + '/';
		m_location->setText(m_dir+bare());
	}
}

void KileNewProjectDlg::makeProjectPath()
{
	m_filename=bare();
	KILE_DEBUG() << "BEFORE " << QFileInfo(location()).absoluteFilePath() << " " << QFileInfo(location()).path() << endl;
	m_dir = QFileInfo(location()).path();
	if (m_dir.right(1) != "/") m_dir = m_dir + '/';

	KILE_DEBUG() << "LOCATION " << location() << " AND " << m_dir << endl;
	m_location->setText(m_dir+m_filename);
}

void KileNewProjectDlg::slotOk()
{
	if ( ! acceptUserExtensions() )
		return;

	//replace ~ and environment variables in the paths
	KUrlCompletion uc;
	uc.setReplaceEnv(true);
	uc.setReplaceHome(true);
	m_location->setText(uc.replacedPath(location()));
	m_file->setText(uc.replacedPath(file()));

	if ( projectTitle().trimmed().isEmpty())
	{
		if (KMessageBox::warningYesNo(this, i18n("You did not enter a project name, if you continue the project name will be set to: Untitled."), i18n("No Name")) == KMessageBox::Yes)
			m_title->setText(i18n("Untitled"));
		else
			return;
	}

	if ( location().trimmed().isEmpty() )
	{
		KMessageBox::error(this, i18n("Please enter the location where the project file should be save to. Also make sure it ends with .kilepr ."), i18n("Empty Location"));
		return;
	}

	QFileInfo fi(location().trimmed());
	QFileInfo dr(fi.path());
	QDir dir = dr.dir();

	if ( location().trimmed().right(7) != ".kilepr")
	{
		KMessageBox::error(this, i18n("The extension of the project filename is not .kilepr , please correct the extension"), i18n("Wrong Filename Extension"));
		return;
	}
	else
	{

		if (dir.isRelative())
		{
			KMessageBox::error(this, i18n("The path for the project file is not an absolute path, absolute paths always begin with an /"),i18n("Relative Path"));
			return;
		}

		KILE_DEBUG() << "==KileNewProjectDlg::slotOk()==============" << endl;
		KILE_DEBUG() << "\t" << location() << " " << fi.path() << endl;
		if (! dr.exists())
		{
			bool suc = true;
			QStringList dirs = QStringList::split("/", fi.path());
			QString path;

			for (uint i=0; i < dirs.count(); ++i)
			{
				path += '/' + dirs[i];
				dir.setPath(path);
				KILE_DEBUG() << "\tchecking : " << dir.absolutePath() << endl;
				if ( ! dir.exists() )
				{
					dir.mkdir(dir.absolutePath());
					suc = dir.exists();
					KILE_DEBUG() << "\t\tcreated : " << dir.absolutePath() << " suc = " << suc << endl;
				}

				if (!suc)
				{
					KMessageBox::error(this, i18n("Could not create the project folder, check your permissions."));
					return;
				}
			}
		}

		if (! dr.isWritable())
		{
			KMessageBox::error(this, i18n("The project folder is not writable, check your permissions."));
			return;
		}
	}

	if ( createNewFile() )
	{
		if ( file().trimmed().isEmpty())
		{
			KMessageBox::error(this, i18n("Please enter a filename for the file that should be added to this project."),
				i18n("No File Name Given"));
			return;
		}

		//check for validity of name first, then check for existence (fixed by tbraun)
		KUrl fileURL; fileURL.setFileName(file());
		KUrl validURL = KileDocument::Info::makeValidTeXURL(fileURL,m_extmanager->isTexFile(fileURL),true);
		if ( validURL != fileURL )
			m_file->setText(validURL.fileName());

		if ( QFileInfo( QDir(fi.path()) , file().trimmed()).exists() )
		{
			if (KMessageBox::warningYesNo(this, i18n("The file \"%1\" already exists, overwrite it?").arg(file()),
				i18n("File Already Exists")) == KMessageBox::No)
				return;
		}
	}

	if (QFileInfo(location()).exists())
	{
		KMessageBox::error(this, i18n("The project file already exists, please select another name. Delete the existing project file if your intention was to overwrite it."),i18n("Project File Already Exists"));
		return;
	}

	accept();
}

void KileNewProjectDlg::fillProjectDefaults()
{
	m_dir = KileConfig::defaultProjectLocation();
	if( !m_dir.endsWith("/") )
		m_dir += '/';
	KILE_DEBUG() << "M_DIR " << m_dir << endl;
	m_location->setText(m_dir);
	m_cb->setChecked(true);

	KileProjectDlgBase::fillProjectDefaults();
}

TemplateItem* KileNewProjectDlg::getSelection() const
{
	return static_cast<TemplateItem*>(m_templateIconView->currentItem());
}

/*
 * KileProjectOptionsDlg
 */
KileProjectOptionsDlg::KileProjectOptionsDlg(KileProject *project, KileDocument::Extensions *extensions, QWidget *parent, const char * name) :
 	KileProjectDlgBase(i18n("Project Options"), extensions, parent, name )
{
	// Layout
	Q3VBoxLayout *vbox = new Q3VBoxLayout(plainPage(), 6,6 );

	m_pgrid->addWidget(m_plabel, 0,0);
	m_pgrid->addWidget(m_title, 0,2);
//	m_pgrid->addWidget(labelEncoding, 1,0);
//	m_pgrid->addWidget(m_lbEncoding, 1,2);
	m_pgrid->setColSpacing(1,8);
	m_pgrid->setColStretch(3,1);
	// second groupbox

	m_egrid->addWidget(m_sel_extensions, 6,0);
	m_egrid->addMultiCellWidget(m_extensions, 6,6, 1,3);
	m_egrid->addWidget(m_lbPredefinedExtensions, 7,0);
	m_egrid->addMultiCellWidget(m_lbStandardExtensions, 7,7, 1,3);

	// third groupbox
	Q3VGroupBox* group3 = new Q3VGroupBox(i18n("Properties"), plainPage());
	group3->setColumnLayout(0, Qt::Vertical );
	group3->layout()->setSpacing( 6 );
	group3->layout()->setMargin( 11 );
	Q3GridLayout *grid3 = new Q3GridLayout( group3->layout() );
	grid3->setAlignment( Qt::AlignTop );

	m_master = new KComboBox( false, group3 );
	m_master->setObjectName( "master" );
	//m_master->setDisabled(true);
	QLabel *lb1 = new QLabel(i18n("&Master document:"), group3);
	lb1->setBuddy(m_master);
	lb1->setMinimumWidth( m_sel_extensions->sizeHint().width() );
	Q3WhatsThis::add(m_master, whatsthisMaster);
	Q3WhatsThis::add(lb1,whatsthisMaster);

	m_master->insertItem(i18n("(auto-detect)"));
	Q3PtrListIterator<KileProjectItem> rit(*(project->rootItems()));
	int index = 0;
	while (rit.current())
	{
		if ((*rit)->type() == KileProjectItem::Source)
		{
			m_master->insertItem((*rit)->url().fileName());
			++index;
			if ( (*rit)->url().path() == project->masterDocument() )
				m_master->setCurrentItem(index);
		}
		++rit;
	}

	if (project->masterDocument().isNull())
		m_master->setCurrentItem(0);

	QLabel *lb2 = new QLabel(i18n("&QuickBuild configuration:"), group3); 
	m_cbQuick = new KComboBox(group3); 
	lb2->setBuddy(m_cbQuick);
	m_cbQuick->insertItem(tool_default);
	m_cbQuick->insertStringList(KileTool::configNames("QuickBuild", KGlobal::config()));
	m_cbQuick->setCurrentText(project->quickBuildConfig().length() > 0 ? project->quickBuildConfig() : tool_default );

	//don't put this after the call to toggleMakeIndex
	setProject(project, true);

	m_ckMakeIndex = new QCheckBox(i18n("&MakeIndex options"), group3); 
	connect(m_ckMakeIndex, SIGNAL(toggled(bool)), this, SLOT(toggleMakeIndex(bool)));
	m_leMakeIndex = new KLineEdit(group3); 
	m_ckMakeIndex->setChecked(project->useMakeIndexOptions());
	toggleMakeIndex(m_ckMakeIndex->isChecked());

	grid3->addWidget(lb1,0,0);
	grid3->addWidget(m_master,0,1);
	grid3->addWidget(lb2,1,0);
	grid3->addWidget(m_cbQuick,1,1);
	grid3->addWidget(m_ckMakeIndex,2,0);
	grid3->addMultiCellWidget(m_leMakeIndex,2,2,1,2);
	grid3->setColStretch(2,1);

	// add to layout
	vbox->addWidget(m_pgroup);
	vbox->addWidget(m_egroup);
	vbox->addWidget(group3);
	vbox->addStretch();
}

KileProjectOptionsDlg::~KileProjectOptionsDlg()
{
}

void KileProjectOptionsDlg::toggleMakeIndex(bool on)
{
	KILE_DEBUG() << "TOGGLED!" << endl;
	m_leMakeIndex->setEnabled(on);
	m_project->setUseMakeIndexOptions(on);
	m_project->writeUseMakeIndexOptions();
	m_project->readMakeIndexOptions();
	m_leMakeIndex->setText(m_project->makeIndexOptions());
}

void KileProjectOptionsDlg::slotOk()
{
	if ( ! acceptUserExtensions() )
		return;

	this->m_project->setName(m_title->text());

	Q3PtrListIterator<KileProjectItem> rit(*(m_project->rootItems()));
	while (rit.current())
	{
		if ((*rit)->url().fileName() == m_master->currentText() )
			m_project->setMasterDocument((*rit)->url().path());
		++rit;
	}
	if (m_master->currentItem() == 0) m_project->setMasterDocument(QString::null);

	m_val_extensions[m_sel_extensions->currentItem()] = m_extensions->text();

	for (int i = KileProjectItem::Source; i < KileProjectItem::Other; ++i) 
	{
		m_project->setExtensions( (KileProjectItem::Type) i, m_val_extensions[i-1] );
	}

	if ( m_cbQuick->currentText() == tool_default )
		m_project->setQuickBuildConfig("");
	else
		m_project->setQuickBuildConfig(m_cbQuick->currentText());

	m_project->setUseMakeIndexOptions(m_ckMakeIndex->isChecked());
	if ( m_project->useMakeIndexOptions() )
		m_project->setMakeIndexOptions(m_leMakeIndex->text());

	m_project->save();

	accept();
}


#include "kileprojectdlgs.moc"
