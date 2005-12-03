/***************************************************************************
    begin                : Wed Jun 6 2001
    copyright            : (C) 2003 by Jeroen Wijnout
    email                : Jeroen.Wijnhout@kdemail.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef TOOLSOPTIONSDIALOG_H
#define TOOLSOPTIONSDIALOG_H

#include <kdialogbase.h>
#include <kdeversion.h>

#include <ktexteditor/configinterfaceextension.h>
#include <kconfigdialogmanager.h>

#include "configcodecompletion.h"     // code completion (dani)
#include "previewconfigwidget.h"      // QuickPreview (dani)

class QFrame;
//class KSpellConfig;
class KConfig;
//class KConfigSkeleton;
//class KConfigDialogManager;

namespace KileWidget { class ToolConfig; }
class KileWidgetHelpConfig;
class KileWidgetLatexConfig;
class KileWidgetGeneralConfig;
namespace KileTool { class Manager; }

namespace KileDialog
{
	class Config : public KDialogBase
	{
		Q_OBJECT

	public:
		Config( KConfig *config, KileInfo *ki, QWidget* parent = 0);
		~Config();

		virtual void show();

	//signals:
	//	void widgetModified();

	private slots:
		void slotOk();
		void slotCancel();
		void slotChanged();

	//	void slotWidgetModified();

	private:
		// dialog manager
		KConfigDialogManager *m_manager;

//		QFrame* spellingPage;

		KConfig *m_config;
		KileInfo *m_ki;

		bool m_editorSettingsChanged;
		bool m_editorOpened;

		KileWidget::ToolConfig	*toolPage;

		// CodeCompletion (dani)
		ConfigCodeCompletion *completePage;
		KileWidgetPreviewConfig *previewPage;

		KileWidgetHelpConfig *helpPage;
		KileWidgetLatexConfig *latexPage;
		KileWidgetGeneralConfig *generalPage;

		// setup configuration
		void addConfigFolder(const QString &section,const QString &icon);

		void addConfigPage( QWidget *page,
		                    const QString &sectionName,const QString &itemName,
		                    const QString &pixmapName, const QString &header=QString::null,
		                    bool addSpacer = true );

		void setupGeneralOptions();
		void setupTools();
		void setupLatex();
		void setupCodeCompletion();
		void setupQuickPreview();
		void setupHelp();
		void setupEditor();

		// write configuration
		void writeGeneralOptionsConfig();
		void writeToolsConfig();

		// synchronize encoding
		QString readKateEncoding();
		void syncKileEncoding();

		// editor pages
		QPtrList<KTextEditor::ConfigPage> editorPages;
	};
}
#endif
