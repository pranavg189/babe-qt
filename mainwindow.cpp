/*
   Babe - tiny music player
   Copyright (C) 2017  Camilo Higuita
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA

   */


#include "mainwindow.h"
#include "ui_mainwindow.h"



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle(" Babe ... \xe2\x99\xa1  \xe2\x99\xa1 \xe2\x99\xa1 ");
    this->setAcceptDrops(true);
    this->setWindowIcon(QIcon(":Data/data/babe_48.svg"));
    this->setWindowIconText("Babe...");

    //mpris = new Mpris(this);

    connect(this, SIGNAL(finishedPlayingSong(QString)),this,SLOT(addToPlayed(QString)));
    connect(this,SIGNAL(getCover(QString,QString,QString)),this,SLOT(setCoverArt(QString,QString,QString)));
    connect(this,SIGNAL(collectionChecked()),this,SLOT(refreshTables()));


    this->setUpViews();
    this->setUpSidebar();
    this->setUpPlaylist();
    this->setUpCollectionViewer();

    if(settings_widget->checkCollection())
    {
        settings_widget->collection_db.setCollectionLists();
        populateMainList();
        emit collectionChecked();
    }
    else settings_widget->createCollectionDB();


    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, [this]() {
        timer->stop();
        infoTable->getTrackInfo(current_title,current_artist,current_album);

    });

    connect(updater, SIGNAL(timeout()), this, SLOT(update()));
    player->setVolume(100);


    /*LOAD THE STYLE*/
    QFile styleFile(stylePath);
    if(styleFile.exists())
    {
        qDebug()<<"A Babe style file exists";
        styleFile.open( QFile::ReadOnly );
        // Apply the loaded stylesheet
        QString style( styleFile.readAll() );
        this->setStyleSheet( style );
    }

    mainList->setCurrentCell(0,BabeTable::TITLE);

    if(mainList->rowCount()!=0)
    {
        loadTrack();
        collectionView();
        go_mini();

    }else
    {
        addMusicImg->setVisible(true);
        collectionView();
    }

    updater->start(1000);
}





MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::setUpViews()
{
    settings_widget = new settings(this); //this needs to go first


    playlistTable = new PlaylistsView(this);
    connect(playlistTable,SIGNAL(playlistCreated(QString, QString)),&settings_widget->getCollectionDB(),SLOT(insertPlaylist(QString, QString)));
    connect(playlistTable->table,SIGNAL(tableWidget_doubleClicked(QList<QStringList>)),this,SLOT(addToPlaylist(QList<QStringList>)));
    connect(playlistTable->table,SIGNAL(removeIt_clicked(int)),this,SLOT(removeSong(int)));
    connect(playlistTable->table,SIGNAL(babeIt_clicked(QList<QStringList>)),this,SLOT(babeIt(QList<QStringList>)));
    //connect(playlistTable->table,SIGNAL(createPlaylist_clicked()),this,SLOT(playlistsView()));
    connect(playlistTable->table,SIGNAL(queueIt_clicked(QString)),this,SLOT(addToQueue(QString)));
    connect(playlistTable->table,SIGNAL(moodIt_clicked(QString)),playlistTable,SLOT(createMoodPlaylist(QString)));
    connect(playlistTable->table,SIGNAL(infoIt_clicked(QString, QString, QString)),this,SLOT(infoIt(QString, QString, QString)));


    collectionTable = new BabeTable(this);
    collectionTable->passCollectionConnection(&settings_widget->getCollectionDB());
    connect(collectionTable,SIGNAL(tableWidget_doubleClicked(QList<QStringList>)),this,SLOT(addToPlaylist(QList<QStringList>)));
    connect(collectionTable,SIGNAL(enteredTable()),this,SLOT(hideControls()));
    connect(collectionTable,SIGNAL(leftTable()),this,SLOT(showControls()));
    connect(collectionTable,SIGNAL(finishedPopulating()),this,SLOT(orderTables()));
    connect(collectionTable,SIGNAL(removeIt_clicked(int)),this,SLOT(removeSong(int)));
    connect(collectionTable,SIGNAL(babeIt_clicked(QList<QStringList>)),this,SLOT(babeIt(QList<QStringList>)));
    connect(collectionTable,SIGNAL(queueIt_clicked(QString)),this,SLOT(addToQueue(QString)));
    connect(collectionTable,SIGNAL(moodIt_clicked(QString)),playlistTable,SLOT(createMoodPlaylist(QString)));
    connect(collectionTable,SIGNAL(infoIt_clicked(QString, QString, QString)),this,SLOT(infoIt(QString, QString, QString)));


    mainList = new BabeTable(this);
    mainList->hideColumn(BabeTable::ALBUM);
    mainList->hideColumn(BabeTable::ARTIST);
    mainList->horizontalHeader()->setVisible(false);
    mainList->setFixedWidth(200);
    mainList->setMinimumHeight(200);
    connect(mainList,SIGNAL(tableWidget_doubleClicked(QList<QStringList>)),this,SLOT(on_mainList_clicked(QList<QStringList>)));
    connect(mainList,SIGNAL(removeIt_clicked(int)),this,SLOT(removeSong(int)));
    connect(mainList,SIGNAL(babeIt_clicked(QList<QStringList>)),this,SLOT(babeIt(QList<QStringList>)));
    connect(mainList,SIGNAL(queueIt_clicked(QString)),this,SLOT(addToQueue(QString)));
    connect(mainList,SIGNAL(moodIt_clicked(QString)),playlistTable,SLOT(createMoodPlaylist(QString)));
    connect(mainList,SIGNAL(infoIt_clicked(QString, QString, QString)),this,SLOT(infoIt(QString, QString, QString)));
    connect(mainList->model() ,SIGNAL(rowsInserted(QModelIndex,int,int)),this,SLOT(on_rowInserted(QModelIndex,int,int)));


    resultsTable=new BabeTable(this);
    //resultsTable->passStyle("QHeaderView::section { background-color:#474747; }");
    resultsTable->setVisibleColumn(BabeTable::STARS);
    resultsTable->showColumn(BabeTable::GENRE);
    connect(resultsTable,SIGNAL(tableWidget_doubleClicked(QList<QStringList>)),this,SLOT(addToPlaylist(QList<QStringList>)));
    connect(resultsTable,SIGNAL(enteredTable()),this,SLOT(hideControls()));
    connect(resultsTable,SIGNAL(leftTable()),this,SLOT(showControls()));
    connect(resultsTable,SIGNAL(removeIt_clicked(int)),this,SLOT(removeSong(int)));
    connect(resultsTable,SIGNAL( babeIt_clicked(QList<QStringList>)),this,SLOT(babeIt(QList<QStringList>)));
    connect(resultsTable,SIGNAL(queueIt_clicked(QString)),this,SLOT(addToQueue(QString)));
    connect(resultsTable,SIGNAL(moodIt_clicked(QString)),playlistTable,SLOT(createMoodPlaylist(QString)));
    connect(resultsTable,SIGNAL(infoIt_clicked(QString, QString, QString)),this,SLOT(infoIt(QString, QString, QString)));


    queueTable = new BabeTable(this);
    queueTable->setSortingEnabled(false);
    connect(queueTable,SIGNAL(babeIt_clicked(QList<QStringList>)),this,SLOT(babeIt(QList<QStringList>)));
    connect(queueTable,SIGNAL(removeIt_clicked(int)),this,SLOT(removeSong(int)));
    connect(queueTable,SIGNAL(moodIt_clicked(QString)),playlistTable,SLOT(createMoodPlaylist(QString)));
    connect(queueTable,SIGNAL(infoIt_clicked(QString, QString, QString)),this,SLOT(infoIt(QString, QString, QString)));


    albumsTable = new AlbumsView(false,this);
    connect(albumsTable,SIGNAL(albumOrderChanged(QString)),this,SLOT(AlbumsViewOrder(QString)));
    connect(albumsTable->albumTable,SIGNAL(tableWidget_doubleClicked(QList<QStringList>)),this,SLOT(addToPlaylist(QList<QStringList>)));
    connect(albumsTable->albumTable,SIGNAL(removeIt_clicked(int)),this,SLOT(removeSong(int)));
    connect(albumsTable->albumTable,SIGNAL( babeIt_clicked(QList<QStringList>)),this,SLOT(babeIt(QList<QStringList>)));
    connect(albumsTable,SIGNAL(playAlbum(QString, QString)),this,SLOT(putOnPlay(QString, QString)));
    connect(albumsTable,SIGNAL(babeAlbum_clicked(QList<QStringList>)),this,SLOT(babeIt(QList<QStringList>)));
    connect(albumsTable->albumTable,SIGNAL(queueIt_clicked(QString)),this,SLOT(addToQueue(QString)));
    connect(albumsTable->albumTable,SIGNAL(moodIt_clicked(QString)),playlistTable,SLOT(createMoodPlaylist(QString)));
    connect(albumsTable->albumTable,SIGNAL(infoIt_clicked(QString, QString, QString)),this,SLOT(infoIt(QString, QString, QString)));


    artistsTable = new AlbumsView(true,this);
    artistsTable->albumTable->showColumn(BabeTable::ALBUM);
    connect(artistsTable->albumTable,SIGNAL(tableWidget_doubleClicked(QList<QStringList>)),this,SLOT(addToPlaylist(QList<QStringList>)));
    connect(artistsTable->albumTable,SIGNAL(removeIt_clicked(int)),this,SLOT(removeSong(int)));
    connect(artistsTable->albumTable,SIGNAL( babeIt_clicked(QList<QStringList>)),this,SLOT(babeIt(QList<QStringList>)));
    connect(artistsTable,SIGNAL(playAlbum(QString, QString)),this,SLOT(putOnPlay(QString, QString)));
    connect(artistsTable,SIGNAL(babeAlbum_clicked(QList<QStringList>)),this,SLOT(babeIt(QList<QStringList>)));
    connect(artistsTable->albumTable,SIGNAL(queueIt_clicked(QString)),this,SLOT(addToQueue(QString)));
    connect(artistsTable->albumTable,SIGNAL(moodIt_clicked(QString)),playlistTable,SLOT(createMoodPlaylist(QString)));
    connect(artistsTable->albumTable,SIGNAL(infoIt_clicked(QString, QString, QString)),this,SLOT(infoIt(QString, QString, QString)));


    infoTable = new InfoView(this);
    connect(infoTable,SIGNAL(playAlbum(QString, QString)),this,SLOT(putOnPlay(QString, QString)));


    settings_widget->readSettings();
    setToolbarIconSize(settings_widget->getToolbarIconSize());
    connect(settings_widget, SIGNAL(toolbarIconSizeChanged(int)), this, SLOT(setToolbarIconSize(int)));
    connect(settings_widget, SIGNAL(collectionDBFinishedAdding(bool)), this, SLOT(collectionDBFinishedAdding(bool)));
    connect(settings_widget, SIGNAL(dirChanged(QString,QString)),this, SLOT(scanNewDir(QString, QString)));
    //connect(settings_widget, SIGNAL(collectionPathRemoved(QString)),&settings_widget->getCollectionDB(), SLOT(removePath(QString)));
    connect(settings_widget, SIGNAL(refreshTables()),this, SLOT(refreshTables()));


    connect(ui->tracks_view, SIGNAL(clicked()), this, SLOT(collectionView()));
    connect(ui->albums_view, SIGNAL(clicked()), this, SLOT(albumsView()));
    connect(ui->artists_view, SIGNAL(clicked()), this, SLOT(favoritesView()));
    connect(ui->playlists_view, SIGNAL(clicked()), this, SLOT(playlistsView()));
    connect(ui->queue_view, SIGNAL(clicked()), this, SLOT(queueView()));
    connect(ui->info_view, SIGNAL(clicked()), this, SLOT(infoView()));
    connect(ui->settings_view, SIGNAL(clicked()), this, SLOT(settingsView()));

    views = new QStackedWidget(this);
    views->setFrameShape(QFrame::NoFrame);
    views->addWidget(collectionTable);
    views->addWidget(albumsTable);
    views->addWidget(artistsTable);
    views->addWidget(playlistTable);
    views->addWidget(queueTable);
    views->addWidget(infoTable);
    views->addWidget(settings_widget);
    views->addWidget(resultsTable);

}

void MainWindow::setUpSidebar()
{
    auto *left_spacer = new QWidget(this);
    left_spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *right_spacer = new QWidget(this);
    right_spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    ui->mainToolBar->setContentsMargins(0,0,0,0);
    ui->mainToolBar->layout()->setMargin(0);
    ui->mainToolBar->layout()->setSpacing(0);
    ui->mainToolBar->setStyleSheet(QString("QToolBar {margin:0; background-color:rgba( 0, 0, 0, 0); background-image:url('%1');} QToolButton{ border-radius:0;}"
                                           " QToolButton:checked{border-radius:0; background: %2}").arg(":Data/data/pattern.png",this->palette().color(QPalette::Highlight).name()));

    //ui->mainToolBar->setStyleSheet(QString("QToolBar{margin:0 background-image:url('%1') repeat; }QToolButton{ border-radius:0;} QToolButton:checked{border-radius:0; background: rgba(0,0,0,50)}").arg(":Data/data/pattern.png"));
    ui->mainToolBar->setOrientation(Qt::Vertical);
    ui->mainToolBar->addWidget(left_spacer);

    ui->tracks_view->setToolTip("Collection");
    ui->mainToolBar->addWidget(ui->tracks_view);

    ui->albums_view->setToolTip("Albums");
    ui->mainToolBar->addWidget(ui->albums_view);

    ui->artists_view->setToolTip("Artists");
    ui->mainToolBar->addWidget(ui->artists_view);

    ui->playlists_view->setToolTip("Playlists");
    ui->mainToolBar->addWidget(ui->playlists_view);

    ui->queue_view->setToolTip("Queue");
    ui->mainToolBar->addWidget(ui->queue_view);

    ui->info_view->setToolTip("Info");
    ui->mainToolBar->addWidget(ui->info_view);

    ui->settings_view->setToolTip("Setings");
    ui->mainToolBar->addWidget(ui->settings_view);

    ui->mainToolBar->addWidget(right_spacer);

}

void MainWindow::setUpCollectionViewer()
{
    layout = new QGridLayout();

    frame_layout = new QGridLayout();
    frame_layout->setContentsMargins(0,0,0,0);
    frame_layout->setSpacing(0);

    frame = new QFrame(this);
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setFrameShadow(QFrame::Raised);
    frame->setLayout(frame_layout);

    line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Plain);
    line->setMaximumHeight(1);

    lineV = new QFrame(this);
    lineV->setFrameShape(QFrame::VLine);
    lineV->setFrameShadow(QFrame::Plain);
    lineV->setMaximumWidth(1);


    utilsBar = new QToolBar(this);
    utilsBar->setMovable(false);
    utilsBar->setContentsMargins(0,0,0,0);
    utilsBar->setStyleSheet("margin:0;");

    utilsBar->addWidget(infoTable->infoUtils);
    utilsBar->addWidget(playlistTable->btnContainer);
    utilsBar->addWidget(ui->searchFrame);
    utilsBar->addWidget(albumsTable->utilsFrame);
    utilsBar->addWidget(ui->collectionUtils);

    utilsBar->actions().at(PLAYLISTS_UB)->setVisible(false); ui->frame_3->hide();
    utilsBar->actions().at(INFO_UB)->setVisible(false);
    utilsBar->actions().at(ALBUMS_UB)->setVisible(false);

    ui->search->setClearButtonEnabled(true);
    ui->search->setPlaceholderText("Search...");

    saveResults_menu = new QMenu(this);
    connect(saveResults_menu, SIGNAL(triggered(QAction*)), this, SLOT(saveResultsTo(QAction*)));
    ui->saveResults->setMenu(saveResults_menu);
    ui->saveResults->setStyleSheet("QToolButton::menu-indicator { image: none; }");

    frame_layout->addWidget(ui->mainToolBar,0,0,3,1,Qt::AlignLeft);
    frame_layout->addWidget(lineV,0,1,3,1,Qt::AlignLeft);
    frame_layout->addWidget(views,0,2);
    frame_layout->addWidget(line,1,2);
    frame_layout->addWidget(utilsBar,2,2);

    layout->addWidget(frame, 0,0);
    layout->addWidget(album_art_frame,0,1, Qt::AlignRight);

    main_widget= new QWidget(this);
    main_widget->setLayout(layout);
    this->setCentralWidget(main_widget);
}

void MainWindow::setUpPlaylist()
{
    /*auto *album_widget= new QWidget();
    album_widget->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Expanding);
    album_widget->setStyleSheet("QWidget { padding:0; margin:0;  }");
    album_widget->setFixedWidth(200); */


    auto *album_view = new QGridLayout();
    album_view->setContentsMargins(0,0,0,0);
    album_view->setSpacing(0);

    album_art_frame=new QFrame();
    album_art_frame->setLayout(album_view);
    album_art_frame->setFrameShadow(QFrame::Raised);
    album_art_frame->setFrameShape(QFrame::StyledPanel);

    album_art = new Album(":Data/data/babe.png",200,0,true,false);
    connect(album_art,SIGNAL(playAlbum(QString , QString)),this,SLOT(putOnPlay(QString, QString)));
    connect(album_art,SIGNAL(changedArt(QString, QString , QString)),this,SLOT(changedArt(QString, QString, QString)));
    connect(album_art,SIGNAL(babeAlbum_clicked(QString, QString)),this,SLOT(babeAlbum(QString, QString)));

    album_art->setFixedSize(200,200);
    album_art->setTitleGeometry(0,0,200,30);
    album_art->titleVisible(false);

    ui->controls->setParent(album_art);
    ui->controls->setGeometry(0,200-50,200,50);

    seekBar = new QSlider(this);
    connect(seekBar,SIGNAL(sliderMoved(int)),this,SLOT(on_seekBar_sliderMoved(int)));

    seekBar->setMaximum(1000);
    seekBar->setOrientation(Qt::Horizontal);
    seekBar->setContentsMargins(0,0,0,0);
    seekBar->setFixedHeight(5);
    seekBar->setStyleSheet(QString("QSlider { background:transparent;} QSlider::groove:horizontal {border: none; background: transparent; height: 5px; border-radius: 0; } QSlider::sub-page:horizontal { background: %1;border: none; height: 5px;border-radius: 0;} QSlider::add-page:horizontal {background: transparent; border: none; height: 5px; border-radius: 0; } QSlider::handle:horizontal {background: %1; width: 8px; } QSlider::handle:horizontal:hover {background: qlineargradient(x1:0, y1:0, x2:1, y2:1,stop:0 #fff, stop:1 #ddd);border: 1px solid #444;border-radius: 4px;}QSlider::sub-page:horizontal:disabled {background: transparent;border-color: #999;}QSlider::add-page:horizontal:disabled {background: transparent;border-color: #999;}QSlider::handle:horizontal:disabled {background: transparent;border: 1px solid #aaa;border-radius: 4px;}").arg(this->palette().color(QPalette::Highlight).name()));

    addMusicImg = new QLabel(mainList);
    addMusicImg->setPixmap(QPixmap(":Data/data/add.png").scaled(120,120,Qt::KeepAspectRatio));
    addMusicImg->setGeometry(45,40,120,120);
    addMusicImg->setVisible(false);

    ui->filterBox->setVisible(false);
    ui->filter->setClearButtonEnabled(true);
    ui->filterBtn->setChecked(false);

    ui->hide_sidebar_btn->setToolTip("Go Mini");
    ui->shuffle_btn->setToolTip("Shuffle");
    ui->tracks_view_2->setVisible(false);

    refreshBtn_menu = new QMenu(this);
    ui->refreshBtn->setMenu(refreshBtn_menu);
    ui->refreshBtn->setPopupMode(QToolButton::MenuButtonPopup);

    auto clearIt = new QAction("Clear list...");
    connect(clearIt, &QAction::triggered, [this]() {
        this->mainList->flushTable();
        lCounter=-1;
    });
    refreshBtn_menu->addAction(clearIt);


    auto unBabeItAll = new QAction("Save as playlist...");
    refreshBtn_menu->addAction(unBabeItAll);
    auto changeIt = new QAction("Change Playlist...");
    refreshBtn_menu->addAction(changeIt);


    album_view->addWidget(album_art, 0,0,Qt::AlignTop);
    album_view->addWidget(ui->frame_6,1,0);
    album_view->addWidget(seekBar,2,0);
    album_view->addWidget(ui->frame_4,3,0);
    album_view->addWidget(mainList,4,0);
    album_view->addWidget(ui->frame_5,5,0);
    album_view->addWidget(ui->playlistUtils,6,0);


}

void MainWindow::changedArt(QString path, QString artist, QString album)
{
    settings_widget->collection_db.execQuery(QString("UPDATE albums SET art = \"%1\" WHERE title = \"%2\" AND artist = \"%3\"").arg(path,album,artist) );

}


void MainWindow::putOnPlay(QString artist, QString album)
{
    if(!artist.isEmpty()||!album.isEmpty())
    {
        qDebug()<<"put on play<<"<<artist<<album;
        //updater->stop();
        // player->stop();


        QSqlQuery query;

        QList<QStringList> list;
        if(album.isEmpty())
            query = settings_widget->collection_db.getQuery("SELECT * FROM tracks WHERE artist = \""+artist+"\" ORDER by album asc, track asc");
        else if(!album.isEmpty()&&!artist.isEmpty())
            query = settings_widget->collection_db.getQuery("SELECT * FROM tracks WHERE artist = \""+artist+"\" AND album = \""+album+"\" ORDER by track asc");

        if(query.exec())
        {
            qDebug()<<"put on play<<2";

            while(query.next())
            {
                QStringList track;
                track<<query.value(BabeTable::TRACK).toString();
                track<<query.value(BabeTable::TITLE).toString();
                track<<query.value(BabeTable::ARTIST).toString();
                track<<query.value(BabeTable::ALBUM).toString();
                track<<query.value(BabeTable::GENRE).toString();
                track<<query.value(BabeTable::LOCATION).toString();
                track<<query.value(BabeTable::STARS).toString();
                track<<query.value(BabeTable::BABE).toString();
                track<<query.value(BabeTable::ART).toString();
                track<<query.value(BabeTable::PLAYED).toString();
                track<<query.value(BabeTable::PLAYLIST).toString();

                list<<track;

            }
            qDebug()<<"put on play<<3";
            if(!list.isEmpty())
            {
                mainList->flushTable();
                currentList.clear();

                addToPlaylist(list);

                if(mainList->rowCount() != 0)
                {
                    qDebug()<<"put on play<<3";
                    mainList->setCurrentCell(0,BabeTable::TITLE);
                    lCounter=0;
                    loadTrack();
                    //player->pause();
                    // updater->start();
                    qDebug()<<"put on play<<4";
                }
            }
        }
    }

}

void MainWindow::addToPlayed(QString url)
{
    if(settings_widget->getCollectionDB().checkQuery("SELECT * FROM tracks WHERE location = \""+url+"\""))
    {
        //ui->fav_btn->setIcon(QIcon::fromTheme("face-in-love"));
        qDebug()<<"Song totally played"<<url;


        QSqlQuery query = settings_widget->getCollectionDB().getQuery("SELECT * FROM tracks WHERE location = \""+url+"\"");

        int played = 0;
        while (query.next()) played = query.value(BabeTable::PLAYED).toInt();
        qDebug()<<played;

        if(settings_widget->getCollectionDB().insertInto("tracks","played",url,played+1))
        {
            //ui->fav_btn->setIcon(QIcon(":Data/data/love-amarok.svg"));
            qDebug()<<played;

        }

    }
}

void MainWindow::resizeEvent(QResizeEvent* event)
{

    //qDebug()<<event->size().width()<<"x"<<event->size().height();
    if(mini_mode==0 && event->size().width()<this->minimumSize().width()+20)
    {
        //this->setMaximumWidth(200);
        // this->setFixedWidth(200);
        //int oldHeight = this->size().height();
        //this->resize(200,oldHeight);
        event->accept();
        go_mini();
    }else if(mini_mode!=0 && event->size().width()!=200)
    {
        int oldHeight = this->size().height();
        this->resize(700,oldHeight);
        event->accept();
        expand();
    }
    QMainWindow::resizeEvent(event);
}

void MainWindow::refreshTables()
{
    collectionTable->flushTable();
    collectionTable->populateTableView("SELECT * FROM tracks");
    // favoritesTable->flushTable();
    //favoritesTable->populateTableView("SELECT * FROM tracks WHERE stars > \"0\" OR babe =  \"1\"");
    //albumsTable->flushGrid();
    albumsTable->populateTableView(settings_widget->getCollectionDB().getQuery("SELECT * FROM albums ORDER by title asc"));
    albumsTable->hideAlbumFrame();
    //artistsTable->flushGrid();
    artistsTable->populateTableViewHeads(settings_widget->getCollectionDB().getQuery("SELECT * FROM artists ORDER by title asc"));
    artistsTable->hideAlbumFrame();

    playlistTable->list->clear();
    playlistTable->setDefaultPlaylists();
    QStringList playLists =settings_widget->getCollectionDB().getPlaylists();
    playlistTable->definePlaylists(playLists);
    playlistTable->setPlaylists(playLists);
    QStringList playListsMoods =settings_widget->getCollectionDB().getPlaylistsMoods();
    playlistTable->defineMoods(playListsMoods);
    playlistTable->setPlaylistsMoods(playListsMoods);
}


void MainWindow::AlbumsViewOrder(QString order)
{
    albumsTable->flushGrid();
    albumsTable->populateTableView(settings_widget->getCollectionDB().getQuery("SELECT * FROM albums ORDER by "+order.toLower()+" asc"));
}


void MainWindow::keyPressEvent(QKeyEvent *event)
{

    QMainWindow::keyPressEvent(event);
    /*switch (event->key())
    {
    case Qt::Key_Return :
    {
        lCounter = getIndex();
        if(lCounter != -1)
        {
            //ui->play->setChecked(false);
            ui->play_btn->setIcon(QIcon(":Data/data/media-playback-pause.svg"));
            ui->search->clear();
            loadTrack();
        }
        break;
    }
    case Qt::Key_Up :
    {
        int ind = getIndex() - 1;if(ind < 0)ind = ui->listWidget->count() - 1;
        ui->listWidget->setCurrentRow(ind);
        break;
    }
    case Qt::Key_Down :
    {
        int ind = getIndex() + 1;if(ind >= ui->listWidget->count())ind = 0;
        ui->listWidget->setCurrentRow(ind);
        break;
    }
    default :
    {
        //ui->search->setFocusPolicy(Qt::StrongFocus);
        //ui->search->setFocus();
        qDebug()<<"trying to focus the serachbar";
        break;
    }
    }*/
}


void MainWindow::enterEvent(QEvent *event)
{
    //qDebug()<<"entered the window";
    //Q_UNUSED(event);
    event->accept();
    showControls();
    //if (mini_mode==2) this->setWindowState(Qt::WindowActive);
}


void MainWindow::leaveEvent(QEvent *event)
{
    //qDebug()<<"left the window";
    // Q_UNUSED(event);
    event->accept();
    hideControls();
    //timer = new QTimer(this);
    /*connect(timer, SIGNAL(timeout()), this, SLOT(hide ui->controls()));

      connect(timer,SIGNAL(timeout()), this, [&timer, this]() {
          qDebug()<<"ime is up";
          timer->stop();
      });*/

    //timer->start(3000);
}

void MainWindow::hideControls()
{
    //qDebug()<<"ime is up";
    ui->controls->hide();
    album_art->titleVisible(false);
    // timer->stop();*/
}

void MainWindow::showControls()
{
    //qDebug()<<"ime is up";
    ui->controls->show();
    if(mini_mode==2)  album_art->titleVisible(true);
    // if (mini_mode==2)album_art->titleVisible(true);
    // timer->stop();*/
}
void	MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    event->accept();
}

void	MainWindow::dragLeaveEvent(QDragLeaveEvent *event){
    event->accept();
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    event->accept();

    QList<QUrl> urls;
    urls = event->mimeData()->urls();
    QStringList list;
    for( auto url  : urls)
    {
        //qDebug()<<url.path();

        if(QFileInfo(url.path()).isDir())
        {
            //QDir dir = new QDir(url.path());
            QDirIterator it(url.path(), QStringList() << "*.mp4" << "*.mp3" << "*.wav" <<"*.flac" <<"*.ogg" << "*.m4a", QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext())
            {
                list<<it.next();

                //qDebug() << it.next();
            }

        }else if(QFileInfo(url.path()).isFile())
        {
            list<<url.path();
        }
    }

    auto tracks = new Playlist();
    tracks->add(list);

    addToPlaylist(tracks->getTracksData());


}


void MainWindow::dummy()
{
    qDebug()<<"TEST on DUMMYT";
}



void MainWindow::setCoverArt(QString artist, QString album,QString title)
{

    auto coverArt = new ArtWork();
    auto artistHead = new ArtWork;
    connect(coverArt, SIGNAL(coverReady(QByteArray)), this, SLOT(putPixmap(QByteArray)));
    connect(artistHead, SIGNAL(headReady(QByteArray)), infoTable, SLOT(setArtistArt(QByteArray)));
    coverArt->setDataCover(artist,album,title);
    artistHead->setDataHead(artist);

}


void MainWindow::putPixmap(QByteArray array)
{
    if(!array.isEmpty()) album_art->putPixmap(array);
    else  album_art->putPixmap(QString(":Data/data/cover.svg"));
    //infoTable->setAlbumInfo(coverArt->info);

    //delete artwork;

}


void MainWindow::addToFavorites(QStringList list)
{
    // favoritesTable->addRow(list.at(0),list.at(1),list.at(2),list.at(3),list.at(4),list.at(5));
    qDebug()<<list.at(0)<<list.at(1)<<list.at(2)<<list.at(3)<<list.at(4)<<list.at(5);
}

void MainWindow::addToCollection(QStringList list)
{
    collectionTable->addRow(list);
    qDebug()<<list.at(0)<<list.at(1)<<list.at(2)<<list.at(3)<<list.at(4)<<list.at(5);
}

void MainWindow::setToolbarIconSize(int iconSize)
{
    qDebug()<< "Toolbar icons size changed";
    ui->mainToolBar->setIconSize(QSize(iconSize,iconSize));
    //playback->setIconSize(QSize(iconSize,iconSize));
    //utilsBar->setIconSize(QSize(iconSize,iconSize));
    ui->mainToolBar->update();
    //playback->update();
    // this->update();
}




void MainWindow::collectionView()
{
    qDebug()<< "All songs view";
    views->setCurrentIndex(COLLECTION);
    if(mini_mode!=0) expand();

    utilsBar->actions().at(ALBUMS_UB)->setVisible(false);
    utilsBar->actions().at(COLLECTION_UB)->setVisible(true);

    utilsBar->actions().at(PLAYLISTS_UB)->setVisible(false); ui->frame_3->hide();
    utilsBar->actions().at(INFO_UB)->setVisible(false);

    ui->tracks_view->setChecked(true);
    prevIndex=views->currentIndex();
}

void MainWindow::albumsView()
{
    views->setCurrentIndex(ALBUMS);
    //if(hideSearch)utilsBar->show(); line->show();
    if(mini_mode!=0) expand();
    utilsBar->actions().at(ALBUMS_UB)->setVisible(true);;
    utilsBar->actions().at(COLLECTION_UB)->setVisible(true);
    utilsBar->actions().at(PLAYLISTS_UB)->setVisible(false); ui->frame_3->hide();
    utilsBar->actions().at(INFO_UB)->setVisible(false);

    prevIndex=views->currentIndex();
}
void MainWindow::playlistsView()
{
    views->setCurrentIndex(PLAYLISTS);
    if(mini_mode!=0) expand();
    utilsBar->actions().at(ALBUMS_UB)->setVisible(false);
    utilsBar->actions().at(COLLECTION_UB)->setVisible(true);
    utilsBar->actions().at(PLAYLISTS_UB)->setVisible(true); ui->frame_3->show();
    utilsBar->actions().at(INFO_UB)->setVisible(false);


    prevIndex=views->currentIndex();
}
void MainWindow::queueView()
{
    views->setCurrentIndex(QUEUE);
    if(mini_mode!=0) expand();
    utilsBar->actions().at(ALBUMS_UB)->setVisible(false);
    utilsBar->actions().at(COLLECTION_UB)->setVisible(false);
    utilsBar->actions().at(PLAYLISTS_UB)->setVisible(false); ui->frame_3->hide();
    utilsBar->actions().at(INFO_UB)->setVisible(false);


    prevIndex=views->currentIndex();
}
void MainWindow::infoView()
{

    views->setCurrentIndex(INFO);

    if(mini_mode!=0) expand();
    utilsBar->actions().at(ALBUMS_UB)->setVisible(false);
    utilsBar->actions().at(COLLECTION_UB)->setVisible(true);
    utilsBar->actions().at(PLAYLISTS_UB)->setVisible(false);
    utilsBar->actions().at(INFO_UB)->setVisible(true); ui->frame_3->show();

    prevIndex=views->currentIndex();
    //if(!hideSearch)utilsBar->hide(); line->hide();
}
void MainWindow::favoritesView()
{
    views->setCurrentIndex(ARTISTS);
    if(mini_mode!=0) expand();
    utilsBar->actions().at(ALBUMS_UB)->setVisible(false);
    utilsBar->actions().at(COLLECTION_UB)->setVisible(true);
    utilsBar->actions().at(PLAYLISTS_UB)->setVisible(false); ui->frame_3->hide();
    utilsBar->actions().at(INFO_UB)->setVisible(false);


    prevIndex=views->currentIndex();

    /* QString url= QFileDialog::getExistingDirectory();

qDebug()<<url;

    QStringList urlCollection;
//QDir dir = new QDir(url);
    QDirIterator it(url, QStringList() << "*.mp4" << "*.mp3" << "*.wav" <<"*.flac" <<"*.ogg", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        urlCollection<<it.next();

        //qDebug() << it.next();
    }

   // collection.add(urlCollection);
    //updateList();
    populateTableView();*/
}


void MainWindow::settingsView()
{
    views->setCurrentIndex(SETTINGS);
    if(mini_mode!=0) expand();
    //if(!hideSearch) utilsBar->hide(); line->hide();
    utilsBar->actions().at(ALBUMS_UB)->setVisible(false);
    utilsBar->actions().at(COLLECTION_UB)->setVisible(true);
    utilsBar->actions().at(PLAYLISTS_UB)->setVisible(false); ui->frame_3->hide();
    utilsBar->actions().at(INFO_UB)->setVisible(false);


    prevIndex=views->currentIndex();

}

void MainWindow::expand()
{
    ui->tracks_view_2->hide();
    views->show(); frame->show();
    ui->mainToolBar->show();
    utilsBar->show(); line->show();


    album_art_frame->setFrameShadow(QFrame::Raised);
    album_art_frame->setFrameShape(QFrame::StyledPanel);
    layout->setContentsMargins(6,0,6,0);
    this->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    //this->setMinimumSize(750,500);
    //qDebug()<<this->minimumWidth()<<this->minimumHeight();

    this->resize(700,500);
    //this->setMinimumSize(0,0);
    // this->adjustSize();
    ui->hide_sidebar_btn->setToolTip("Go Mini");

    ui->hide_sidebar_btn->setIcon(QIcon(":Data/data/mini_mode.svg"));
    //ui->mainToolBar->actions().at(0)->setVisible(true);
    // ui->mainToolBar->actions().at(8)->setVisible(true);

    mini_mode=0;
    //keepOnTop(false);
}

void MainWindow::go_mini()
{
    //this->setMaximumSize (0, 0);
    /*this->setWindowFlags(Qt::Window | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
this->show();*/
    ui->tracks_view_2->show();

    QString icon;

    switch(prevIndex)
    {
    case COLLECTION: icon="filename-filetype-amarok"; break;
    case ALBUMS:  icon="media-album-track"; break;
    case ARTISTS:  icon="draw-star"; break;
    case PLAYLISTS:  icon="amarok_lyrics"; break;
    case QUEUE:  icon="amarok_clock"; break;
    case INFO: icon="internet-amarok"; break;

    case SETTINGS:  icon="games-config-options"; break;
    default:  icon="search";
    }

    ui->tracks_view_2->setIcon(QIcon::fromTheme(icon));

    views->hide(); frame->hide();
    ui->mainToolBar->hide();
    //playlistTable->line_v->hide();
    utilsBar->hide(); line->hide();

    // hideSearch=true;
    // ui->mainToolBar->actions().at(0)->setVisible(false);
    // ui->mainToolBar->actions().at(8)->setVisible(false);
    album_art_frame->setFrameShadow(QFrame::Plain);
    album_art_frame->setFrameShape(QFrame::NoFrame);
    layout->setContentsMargins(0,0,0,0);
    // ui->utilsBar->setVisible(false);
    //this->setMinimumSize(200,400);
    int oldHeigh = this->size().height();
    this->resize(200,oldHeigh);
    this->setFixedWidth(200);
    //this->adjustSize();
    ui->hide_sidebar_btn->setToolTip("Go Extra-Mini");
    ui->hide_sidebar_btn->setIcon(QIcon(":Data/data/mini_mode.svg"));
    mini_mode=1;
    //keepOnTop(true);

}

void MainWindow::keepOnTop(bool state)
{

    if (state)
    {//Qt::WindowFlags flags = windowFlags();
        setWindowFlags(Qt::WindowStaysOnTopHint);
        show();
    }else
    {
        //setWindowFlags(~ Qt::WindowStaysOnTopHint);
        //show();
    }
}

void MainWindow::setStyle()
{

    /* ui->mainToolBar->setStyleSheet(" QToolBar { border-right: 1px solid #575757; } QToolButton:hover { background-color: #d8dfe0; border-right: 1px solid #575757;}");
    playback->setStyleSheet("QToolBar { border:none;} QToolBar QToolButton { border:none;} QToolBar QSlider { border:none;}");
    this->setStyleSheet("QToolButton { border: none; padding: 5px; }  QMainWindow { border-top: 1px solid #575757; }");*/
    //status->setStyleSheet("QToolButton { color:#fff; } QToolBar {background-color:#575757; color:#fff; border:1px solid #575757;} QToolBar QLabel { color:#fff;}" );

}



void MainWindow::on_hide_sidebar_btn_clicked()
{
    if(mini_mode==0)
    {
        go_mini();

    }else if(mini_mode==1)
    {

        mainList->hide();
        ui->mainToolBar->hide();
        ui->frame_4->hide();
        ui->playlistUtils->hide();
        // ui->mainToolBar->hide();
        // ui->tableWidget->hide();
        //this->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);

        this->setFixedSize(200,200);
        //album_art->border_radius=5;
        album_art->borderColor=true;

        // album_art->setStyleSheet("QLabel{background-color:transparent;}");
        // album_art_frame->setStyleSheet("QFrame{border: 1px solid red;border-radius:5px;}");
        // this->setStyleSheet("QMainWindow{background-color:transparent;");
        //album_widget.
        layout->setContentsMargins(0,0,0,0);

        this->setWindowFlags(this->windowFlags() | Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        this->show();
        ui->hide_sidebar_btn->setToolTip("Expand");

        mini_mode=2;

    }else if(mini_mode==2)
    {

        this->setWindowFlags(this->windowFlags() & ~Qt::WindowStaysOnTopHint & ~Qt::Tool & ~Qt::FramelessWindowHint);
        this->show();
        // ui->mainToolBar->show();
        mainList->show();
        ui->frame_4->show();
        ui->playlistUtils->show();

        ui->hide_sidebar_btn->setToolTip("Full View");
        //layout->setContentsMargins(6,0,6,0);
        // album_art->titleVisible(false);
        // album_art->border_radius=2;
        album_art->borderColor=false;
        //album_art->setStyleSheet("QLabel{border: none}");
        ui->hide_sidebar_btn->setIcon(QIcon(":Data/data/full_mode.svg"));
        ui->mainToolBar->hide();


        //main_widget->resize(minimumSizeHint());

        //this->setMinimumSize(0,0);

        this->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        this->resize(700,500);
        this->setMinimumSize(0,0);
        this->setFixedWidth(200);

        //this->adjustSize();


        //this->updateGeometry();
        //this->setfix(minimumSizeHint());
        //this->adjustSize();
        mini_mode=3;
    }else if (mini_mode==3)
    {
        expand();
    }


}

void MainWindow::on_shuffle_btn_clicked()
{
    /*state 0: media-playlist-consecutive-symbolic
            1: media-playlist-shuffle
            2:media-playlist-repeat-symbolic
    */
    if(shuffle_state==0)
    {
        shuffle = true;
        shufflePlaylist();
        ui->shuffle_btn->setIcon(QIcon(":Data/data/media-playlist-shuffle.svg"));
        ui->shuffle_btn->setToolTip("Repeat");
        shuffle_state=1;

    }else if (shuffle_state==1)
    {

        repeat = true;
        ui->shuffle_btn->setIcon(QIcon(":Data/data/media-playlist-repeat.svg"));
        ui->shuffle_btn->setToolTip("Consecutive");
        shuffle_state=2;


    }else if(shuffle_state==2)
    {
        repeat = false;
        shuffle = false;
        ui->shuffle_btn->setIcon(QIcon(":Data/data/view-media-playlist.svg"));
        ui->shuffle_btn->setToolTip("Shuffle");
        shuffle_state=0;
    }



}

void MainWindow::on_open_btn_clicked()
{
    //bool startUpdater = false;

    //if(ui->listWidget->count() == 0) startUpdater = true;




    QStringList files = QFileDialog::getOpenFileNames(this, tr("Select Music Files"),QDir().homePath()+"/Music/", tr("Audio (*.mp3 *.wav *.mp4 *.flac *.ogg *.m4a)"));
    if(!files.isEmpty())
    {
        auto tracks = new Playlist();
        tracks->add(files);

        addToPlaylist(tracks->getTracksData());
    }
}



void MainWindow::populateMainList()
{
    mainList->populateTableView("SELECT * FROM tracks WHERE babe = 1 ORDER by played desc",true);
    mainList->resizeRowsToContents();
    currentList=mainList->getAllTableContent();
}

void MainWindow::updateList()
{
    mainList->flushTable();

    for(auto list: currentList)
    {
        mainList->addRow(list);
    }

}



void MainWindow::on_mainList_clicked(QList<QStringList> list)
{
    Q_UNUSED(list);
    lCounter = getIndex();

    //ui->play_btn->setChecked(false);
    //ui->searchBar->clear();
    loadTrack();


    playing= true;
    ui->play_btn->setIcon(QIcon(":Data/data/media-playback-pause.svg"));

}

void MainWindow::removeSong(int index)
{

    QObject* obj = sender();

    if(index != -1)
    {

        qDebug()<<"ehat was in current list:";
        for(auto a: currentList)
        {
            qDebug()<<a.at(1);
        }
        if(obj == mainList) currentList.removeAt(index);
        qDebug()<<"ehats in current list:";
        for(auto a: currentList)
        {
            qDebug()<<a.at(1);
        }
        //updateList();
        // mainList->setCurrentCell(index,0);

        if(shuffle) shufflePlaylist();
    }
}

void MainWindow::loadTrack()
{

    if(queue_list.isEmpty())
    {
        int row = getIndex();
        mainList->scrollTo(mainList->model()->index(row,1));

        current_song_url=mainList->model()->data(mainList->model()->index(row, BabeTable::LOCATION)).toString();
        //current_song_url = QString::fromStdString(playlist.tracks[getIndex()].getLocation());
        qDebug()<<"in mainlist="<<current_song_url;

        if(BaeUtils::fileExists(current_song_url))
        {
            current_artist=mainList->model()->data(mainList->model()->index(row, BabeTable::ARTIST)).toString();
            current_album=mainList->model()->data(mainList->model()->index(row, BabeTable::ALBUM)).toString();
            current_title=mainList->model()->data(mainList->model()->index(row, BabeTable::TITLE)).toString();

            timer->start(3000);

            player->setMedia(QUrl::fromLocalFile(current_song_url));
            player->play();

            ui->play_btn->setIcon(QIcon(":Data/data/media-playback-pause.svg"));
            qDebug()<<"Current song playing is: "<< current_song_url;
            this->setWindowTitle(current_title+" \xe2\x99\xa1 "+current_artist);

            album_art->setArtist(current_artist);
            album_art->setAlbum(current_album);
            album_art->setTitle();

            //CHECK IF THE SONG IS BABED IT OR IT ISN'T
            if(settings_widget->getCollectionDB().checkQuery("SELECT * FROM tracks WHERE location = \""+current_song_url+"\" AND babe = \"1\""))
            {
                ui->fav_btn->setIcon(QIcon(":Data/data/loved.svg"));
            }else
            {
                ui->fav_btn->setIcon(QIcon(":Data/data/love-amarok.svg"));
            }



            loadMood();

            //AND WHETHER THE SONG EXISTS OR  DO NOT GET THE TRACK INFO
            loadCover(current_artist,current_album,current_title);

            //if(player->position()>player->duration()/4)




        }else
        {
            removeSong(getIndex());
            qDebug()<<"this song doesn't exists: "<< current_song_url;
        }
    }else
    {
        loadTrackOnQueue();
    }
}


void MainWindow::loadTrackOnQueue()
{
    current_song_url = QString::fromStdString(queueList.tracks[0].getLocation());


    if(BaeUtils::fileExists(current_song_url))
    {
        current_artist=QString::fromStdString(queueList.tracks[0].getArtist());
        current_album=QString::fromStdString(queueList.tracks[0].getAlbum());
        current_title=QString::fromStdString(queueList.tracks[0].getTitle());



        player->setMedia(QUrl::fromLocalFile(current_song_url));
        player->play();

        ui->play_btn->setIcon(QIcon(":Data/data/media-playback-pause.svg"));
        qDebug()<<"Current song playing is: "<< current_song_url;
        this->setWindowTitle(current_title+" \xe2\x99\xa1 "+current_artist);

        album_art->setArtist(current_artist);
        album_art->setAlbum(current_album);
        album_art->setTitle();

        //CHECK IF THE SONG IS BABED IT OR IT ISN'T
        if(settings_widget->getCollectionDB().checkQuery("SELECT * FROM tracks WHERE location = \""+current_song_url+"\" AND babe = \"1\""))
        {
            ui->fav_btn->setIcon(QIcon(":Data/data/loved.svg"));
        }else
        {
            ui->fav_btn->setIcon(QIcon(":Data/data/love-amarok.svg"));
        }


        loadMood();



        //AND WHETHER THE SONG EXISTS OR  DO NOT GET THE TRACK INFO
        loadCover(current_artist,current_album,current_title);

        removeFromQueue(current_song_url);
        //lCounter--;
        //if(player->position()>player->duration()/4)

        timer->start(2000);


    }else
    {
        removeSong(getIndex());
        qDebug()<<"this song doesn't exists: "<< current_song_url;
    }
}

void MainWindow::loadMood()
{
    QString color;
    QSqlQuery query = settings_widget->collection_db.getQuery("SELECT * FROM tracks WHERE location = \""+current_song_url+"\"");
    if(query.exec())
        while (query.next())
            color=query.value(BabeTable::ART).toString();

    if(!color.isEmpty())
    {
        /*QColor mood;::item:selected
         mood.setNamedColor(color);
         mood = mood.lighter(120);*/
        seekBar->setStyleSheet(QString("QSlider\n{\nbackground:transparent;}\nQSlider::groove:horizontal {border: none; background: transparent; height: 5px; border-radius: 0; } QSlider::sub-page:horizontal {\nbackground: %1 ;border: none; height: 5px;border-radius: 0;} QSlider::add-page:horizontal {\nbackground: transparent; border: none; height: 5px; border-radius: 0; } QSlider::handle:horizontal {background: %1; width: 8px; } QSlider::handle:horizontal:hover {background: qlineargradient(x1:0, y1:0, x2:1, y2:1,stop:0 #fff, stop:1 #ddd);border: 1px solid #444;border-radius: 4px;}QSlider::sub-page:horizontal:disabled {background: #bbb;border-color: #999;}QSlider::add-page:horizontal:disabled {background: #eee;border-color: #999;}QSlider::handle:horizontal:disabled {background: #eee;border: 1px solid #aaa;border-radius: 4px;}").arg(color));
        mainList->setStyleSheet(QString("QTableWidget::item:selected {background:rgba( %1, %2, %3, 40); color: %4}").arg(QString::number(QColor(color).toRgb().red()),QString::number(QColor(color).toRgb().green()),QString::number(QColor(color).toRgb().blue()),mainList->palette().color(QPalette::WindowText).name()));
        ui->mainToolBar->setStyleSheet(QString("QToolBar {margin:0; background-color:rgba( %1, %2, %3, 20); background-image:url('%4');} QToolButton{ border-radius:0;} QToolButton:checked{border-radius:0; background: rgba( %1, %2, %3, 155); color: %5;}").arg(QString::number(QColor(color).toRgb().red()),QString::number(QColor(color).toRgb().green()),QString::number(QColor(color).toRgb().blue()),":Data/data/pattern.png",this->palette().color(QPalette::BrightText).name()));

    }else
    {
        //ui->listWidget->setBackgroundRole(QPalette::Highlight);
        //ui->listWidget->setpa
        seekBar->setStyleSheet(QString("QSlider { background:transparent;} QSlider::groove:horizontal {border: none; background: transparent; height: 5px; border-radius: 0; } QSlider::sub-page:horizontal { background: %1;border: none; height: 5px;border-radius: 0;} QSlider::add-page:horizontal {background: transparent; border: none; height: 5px; border-radius: 0; } QSlider::handle:horizontal {background: %1; width: 8px; } QSlider::handle:horizontal:hover {background: qlineargradient(x1:0, y1:0, x2:1, y2:1,stop:0 #fff, stop:1 #ddd);border: 1px solid #444;border-radius: 4px;}QSlider::sub-page:horizontal:disabled {background: transparent;border-color: #999;}QSlider::add-page:horizontal:disabled {background: transparent;border-color: #999;}QSlider::handle:horizontal:disabled {background: transparent;border: 1px solid #aaa;border-radius: 4px;}").arg(this->palette().color(QPalette::Highlight).name()));
        mainList->setStyleSheet(QString("QTableWidget::item:selected {background:%1; color: %2}").arg(this->palette().color(QPalette::Highlight).name(),this->palette().color(QPalette::BrightText).name()));
        ui->mainToolBar->setStyleSheet(QString("QToolBar {margin:0; background-color:rgba( 0, 0, 0, 0); background-image:url('%1');} QToolButton{ border-radius:0;} QToolButton:checked{border-radius:0; background: %2; color:%3;}").arg(":Data/data/pattern.png",this->palette().color(QPalette::Highlight).name(),this->palette().color(QPalette::BrightText).name()));

    }
}


void MainWindow::loadCover(QString artist, QString album, QString title)
{

    Q_UNUSED(title);

    //IF CURRENT SONG EXISTS IN THE COLLECTION THEN GET THE COVER FROM DB
    if(settings_widget->getCollectionDB().checkQuery("SELECT * FROM tracks WHERE location = \""+current_song_url+"\""))
    {
        QSqlQuery queryCover = settings_widget->collection_db.getQuery("SELECT * FROM albums WHERE title = \""+album+"\" AND artist = \""+artist+"\"");
        while (queryCover.next())
        {
            if(!queryCover.value(2).toString().isEmpty()&&queryCover.value(2).toString()!="NULL")
            {

                album_art->putPixmap( queryCover.value(2).toString());
                //mpris->updateCurrentCover(queryCover.value(2).toString());
                if(!this->isActiveWindow())
                {

                    QPixmap pix;
                    if (!queryCover.value(2).toString().isEmpty())  pix.load(queryCover.value(2).toString());

                    //connect(nof,SIGNAL(babeSong(QStringList)),this,SLOT(babeIt(QStringList)));
                    nof.notifySong(title,artist,album,current_song_url,pix);
                }

            }else
            {
                album_art->putPixmap( QString(":Data/data/cover.svg"));
            }

        }

        QSqlQuery queryHead = settings_widget->collection_db.getQuery("SELECT * FROM artists WHERE title = \""+artist+"\"");
        while (queryHead.next())
        {

            if(!queryHead.value(1).toString().isEmpty()&&queryHead.value(1).toString()!="NULL")
            {
                infoTable->artist->putPixmap( queryHead.value(1).toString());
                infoTable->artist->setArtist(artist);
            }else
            {
                infoTable->artist->putPixmap( QString(":Data/data/cover.svg"));
            }

        }


    }else
    {
        qDebug()<<"Song path dirent exits in db so going to get artwork somehowelse <<"<<album<<artist;
        QSqlQuery queryCover = settings_widget->collection_db.getQuery("SELECT * FROM albums WHERE title = \""+album+"\" AND artist = \""+artist+"\"");

        if (queryCover.exec())
        {


            if (queryCover.next())
            {
                if(queryCover.value(0).toString()==album&&queryCover.value(1).toString()==artist)
                {
                    qDebug()<<"found the artwork in cache2";
                    if(!queryCover.value(2).toString().isEmpty()&&queryCover.value(2).toString()!="NULL")
                    {
                        qDebug()<<"found the artwork in cache3";
                        album_art->putPixmap(queryCover.value(2).toString());
                        infoTable->artist->putPixmap(queryCover.value(2).toString());
                        QPixmap pix;
                        pix.load(queryCover.value(2).toString());

                        nof.notifySong(title,artist,album,current_song_url,pix);
                    }else
                    {
                        album_art->putPixmap( QString(":Data/data/cover.svg"));
                    }


                }

            }else
            {
                QPixmap pix;

                nof.notifySong(title,artist,album,current_song_url, pix);
                emit getCover(artist,album,title);
            }
        }



    }
}

void MainWindow::addToQueue(QString url)
{
    qDebug()<<"SONGS IN QUEUE";
    queue_list<<url;
    queueList.add({url});
    queueTable->flushTable();


    nof.notify("Song added to Queue",url);

    for(auto a:queue_list)queueTable->populateTableView("SELECT * FROM tracks WHERE location = \""+a+"\"");
    queueTable->setSortingEnabled(false);
}

void MainWindow::removeFromQueue(QString url)
{
    queue_list.removeAll(url);
    queueList.removeAll();
    queueList.add(queue_list);


    queueTable->flushTable();

    for(auto a:queue_list)queueTable->populateTableView("SELECT * FROM tracks WHERE location = \""+a+"\"");
    queueTable->setSortingEnabled(false);
    // queueList.remove();
}




int MainWindow::getIndex()
{

    return mainList->currentIndex().row();
}



void MainWindow::on_seekBar_sliderMoved(int position)
{
    player->setPosition(player->duration() / 1000 * position);
}






void MainWindow::update()
{
    if(mainList->rowCount()!=0)
    {
        if(!seekBar->isEnabled()) seekBar->setEnabled(true);
        if(addMusicImg->isVisible()) addMusicImg->setVisible(false);
        if(!seekBar->isSliderDown())
            seekBar->setValue((double)player->position()/player->duration() * 1000);

        if(player->state() == QMediaPlayer::StoppedState)
        {
            QString prevSong = current_song_url;
            qDebug()<<"finished playing song: "<<prevSong;
            next();
            emit finishedPlayingSong(prevSong);
        }
    }else
    {
        addMusicImg->setVisible(true);
        if(player->state() != QMediaPlayer::PlayingState)
        {
            seekBar->setValue(0);
            seekBar->setEnabled(false);
        }else
        {
            if(!seekBar->isSliderDown())
                seekBar->setValue((double)player->position()/player->duration() * 1000);
        }

    }
}

void MainWindow::next()
{

    if(queue_list.isEmpty())
    {
        lCounter++;

        if(repeat)
        {
            lCounter--;
        }

        if(lCounter >= mainList->rowCount())
            lCounter = 0;

        (!shuffle or repeat) ? mainList->setCurrentCell(lCounter,0) : mainList->setCurrentCell(shuffledPlaylist[lCounter],0);

        //ui->play->setChecked(false);
        //ui->searchBar->clear();
    }

    loadTrack();


    // timer->setInterval(1000);



}

void MainWindow::back()
{
    lCounter--;

    if(lCounter < 0)
        lCounter = mainList->rowCount() - 1;


    (!shuffle) ? mainList->setCurrentCell(lCounter,0) : mainList->setCurrentCell(shuffledPlaylist[lCounter],0);

    //ui->play->setChecked(false);
    //ui->searchBar->clear();

    loadTrack();

}

void MainWindow::shufflePlaylist()
{
    shuffledPlaylist.resize(0);

    for(int i = 0; i < mainList->rowCount(); i++)
    {
        shuffledPlaylist.push_back(i);
    }

    random_shuffle(shuffledPlaylist.begin(), shuffledPlaylist.end());
}

void MainWindow::on_play_btn_clicked()
{
    if(mainList->rowCount() != 0)
    {
        if(player->state() == QMediaPlayer::PlayingState)
        {
            player->pause();
            ui->play_btn->setIcon(QIcon(":Data/data/media-playback-start.svg"));
        }
        else
        {
            player->play();
            //updater->start();
            ui->play_btn->setIcon(QIcon(":Data/data/media-playback-pause.svg"));
        }
    }
}

void MainWindow::on_backward_btn_clicked()
{
    if(mainList->rowCount() != 0)
    {
        if(player->position() > 3000)
        {
            player->setPosition(0);
        }
        else
        {

            back();
            ui->play_btn->setIcon(QIcon(":Data/data/media-playback-pause.svg"));
        }
    }
}

void MainWindow::on_foward_btn_clicked()
{
    if(mainList->rowCount() != 0)
    {
        if(repeat)
        {
            repeat = !repeat;next();repeat = !repeat;
        }
        else
        {
            next();
            ui->play_btn->setIcon(QIcon(":Data/data/media-playback-pause.svg"));
        }
    }
}


void MainWindow::collectionDBFinishedAdding(bool state)
{
    if(state)
    {
        if(!ui->fav_btn->isEnabled()) ui->fav_btn->setEnabled(true);
        qDebug()<<"now it i time to put the tracks in the table ;)";
        //settings_widget->getCollectionDB().closeConnection();
        albumsTable->flushGrid();
        artistsTable->flushGrid();
        refreshTables();
    }
}

void MainWindow::orderTables()
{
    //favoritesTable->setTableOrder(BabeTable::STARS,BabeTable::DESCENDING);
    collectionTable->setTableOrder(BabeTable::ARTIST,BabeTable::ASCENDING);
    qDebug()<<"finished populating tables, now ordering them";
}





void MainWindow::on_fav_btn_clicked()
{
    babeIt(settings_widget->collection_db.getTrackData({current_song_url}));

}


void MainWindow::babeAlbum(QString album, QString artist)
{
    QSqlQuery query;
    if(album.isEmpty())
        query = settings_widget->getCollectionDB().getQuery("SELECT * FROM tracks WHERE artist = \""+artist+"\" ORDER by album asc, track asc ");
    else if(!artist.isEmpty())
        query = settings_widget->getCollectionDB().getQuery("SELECT * FROM tracks WHERE artist = \""+artist+"\" and album = \""+album+"\" ORDER by track asc ");

    if(query.exec())
    {
        QStringList urls;
        while(query.next())
        {
            urls<<query.value(CollectionDB::LOCATION).toString();
        }

        babeIt(settings_widget->getCollectionDB().getTrackData(urls));


    }
}

void MainWindow::unbabeIt(QString url)
{
    qDebug()<<"The song is already babed";
    if(settings_widget->getCollectionDB().insertInto("tracks","babe",url,0))
    {
        ui->fav_btn->setIcon(QIcon(":Data/data/love-amarok.svg"));

    }
}

void MainWindow::babeIt(QList<QStringList> list)
{

    for(auto track : list)
    {
        QString url = track.at(BabeTable::LOCATION);
        if(settings_widget->getCollectionDB().checkQuery("SELECT * FROM tracks WHERE location = \""+url+"\" AND babe = \"1\""))
        {
            //ui->fav_btn->setIcon(QIcon::fromTheme("face-in-love"));

            unbabeIt(url);

            nof.notify("Song unBabe'd it",url);
        }else
        {

            if(settings_widget->getCollectionDB().check_existance("tracks","location",url))
            {
                if(settings_widget->getCollectionDB().insertInto("tracks","babe",url,1))
                {
                    ui->fav_btn->setIcon(QIcon(":Data/data/loved.svg"));

                    nof.notify("Song Babe'd it",url);
                    QList<QStringList> item;
                    item<<track;
                    addToPlaylist(item,true);

                }
                qDebug()<<"trying to babe sth";
            }else
            {


                qDebug()<<"Sorry but that song is not in the database";


                ui->fav_btn->setIcon(QIcon(":Data/data/loved.svg"));
                ui->fav_btn->setEnabled(false);


                if(addToCollectionDB_t({url},"1"))
                {
                    nof.notify("Song Babe'd it",url);
                }

                //ui->tableWidget->insertRow(ui->tableWidget->rowCount());


                //to-do: create a list and a tracks object and send it the new song and then write that track list into the database
            }


        }
        // addToFavorites({QString::fromStdString(playlist.tracks[getIndex()].getTitle()),QString::fromStdString(playlist.tracks[getIndex()].getArtist()),QString::fromStdString(playlist.tracks[getIndex()].getAlbum()),QString::fromStdString(playlist.tracks[getIndex()].getLocation()),"\xe2\x99\xa1","1"});
    }
}


void  MainWindow::infoIt(QString title, QString artist, QString album)
{
    //views->setCurrentIndex(INFO);
    infoView();
    infoTable->getTrackInfo(title, artist,album);


}




void MainWindow::scanNewDir(QString url,QString babe)
{
    QStringList list;
    qDebug()<<"scanning new dir: "<<url;
    QDirIterator it(url, QStringList() << "*.mp4" << "*.mp3" << "*.wav" <<"*.flac" <<"*.ogg" <<"*.m4a", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        QString song = it.next();
        qDebug()<<song;
        if(QFileInfo(song).isDir())
        {
            qDebug()<<"found the dir";
        }
        if(!settings_widget->getCollectionDB().check_existance("tracks","location",song))
        {
            // qDebug()<<"New music files recently added: "<<it.next();
            list<<song;
        }
        //qDebug() << it.next();
    }

    if (!list.isEmpty())
    {


        if(addToCollectionDB_t(list,babe))
        {

            nof.notify("New music added to collection",listToString(list));
        }

    }
    else {

        refreshTables();
        settings_widget->refreshWatchFiles(); qDebug()<<"a folder probably got removed or changed";
    }


}

QString MainWindow::listToString(QStringList list)
{   QString result;

    for(auto str : list) result+= str+"\n";

    return result;
}


//iterates through the paths of the modify folders tp search for new music and then refreshes the collection view
bool MainWindow::addToCollectionDB_t(QStringList url, QString babe)
{
    if(settings_widget->getCollectionDB().addTrack(url,babe.toInt()))
    {
        if(babe.contains("1"))
        {
            for(auto track: url)
            {

                addToPlaylist(settings_widget->getCollectionDB().getTrackData(url),true);
            }


        }
        return true;
    }else
    {
        return false;
    }


}


void MainWindow::addToPlaylist(QList<QStringList> list, bool notRepeated)
{
    //currentList.clear();
    qDebug()<<"Adding lists to mainPlaylist";
    for(auto a: list)
    {
        qDebug()<<a.at(BabeTable::TITLE);
    }

    //currentList=mainList->getAllTableContent();

    if(notRepeated)
    {
        QList<QStringList> newList;
        QStringList alreadyInList=mainList->getTableContent(BabeTable::LOCATION);
        for(auto track: list)
        {

            if(!alreadyInList.contains(track.at(BabeTable::LOCATION)))
            {
                newList<<track;
                mainList->addRow(track);
            }

        }

        currentList+=newList;
    }else
    {

        currentList+=list;
        for(auto track:list)  mainList->addRow(track);

    }


    mainList->resizeRowsToContents();

    //updateList();
    if(shuffle) shufflePlaylist();
    mainList->scrollToBottom();
}


void  MainWindow::clearCurrentList()
{
    currentList.clear();
    mainList->flushTable();
}

void MainWindow::on_search_returnPressed()
{

    if(ui->search->text().size()!=0) views->setCurrentIndex(RESULTS);
    else views->setCurrentIndex(prevIndex);


}

void MainWindow::on_search_textChanged(const QString &arg1)
{
    QString search=arg1;
    QStringList keys = {"location:","artist:","album:","title:","genre:" };
    QString key;


    for(auto k : keys)
    {

        if(search.contains(k))
        {

            key=k;
            qDebug()<<"search contains key: "<<key;
            search.replace(k,"");
        }
    }


    qDebug()<<"Searching for: "<<search;
    //int oldIndex = views->currentIndex();
    //qDebug()<<oldIndex;
    utilsBar->actions().at(ALBUMS_UB)->setVisible(false);


    if(!search.isEmpty())
    {
        views->setCurrentIndex(RESULTS);
        if(prevIndex==PLAYLISTS) {utilsBar->actions().at(PLAYLISTS_UB)->setVisible(false); ui->frame_3->hide();}

        resultsTable->flushTable();

        if(key=="location:")
        {
            resultsTable->populateTableView("SELECT * FROM tracks WHERE location LIKE \"%"+search+"%\"");
            ui->search->setBackgroundRole(QPalette :: Dark);
        }else if(key== "artist:")
        {
            resultsTable->populateTableView("SELECT * FROM tracks WHERE artist LIKE \"%"+search+"%\"");
            ui->search->setBackgroundRole(QPalette :: Dark);
        }else if(key== "album:")
        {
            resultsTable->populateTableView("SELECT * FROM tracks WHERE album LIKE \"%"+search+"%\"");
            ui->search->setBackgroundRole(QPalette :: Dark);

        }else if(key=="title:")
        {
            resultsTable->populateTableView("SELECT * FROM tracks WHERE title LIKE \"%"+search+"%\"");
            ui->search->setBackgroundRole(QPalette :: Dark);
        }else if(key==  "genre:")
        {
            resultsTable->populateTableView("SELECT * FROM tracks WHERE genre LIKE \"%"+search+"%\"");
            //ui->search->setStyleSheet("background-color:#e3f4d7;");
            ui->search->setBackgroundRole(QPalette :: Dark);
        }else
        {

            resultsTable->populateTableView("SELECT * FROM tracks WHERE title LIKE \"%"+search+"%\" OR artist LIKE \"%"+search+"%\" OR album LIKE \"%"+search+"%\"OR genre LIKE \"%"+search+"%\"");
            //ui->search->setStyleSheet("background-color:transparent;");
            ui->search->setBackgroundRole(QPalette :: Light);
        }

        //ui->saveResults->setEnabled(true);
        ui->refreshAll->setEnabled(false);
        // prevIndex= views->currentIndex();

    }else
    {
        views->setCurrentIndex(prevIndex);
        if(views->currentIndex()==ALBUMS)  utilsBar->actions().at(ALBUMS_UB)->setVisible(true);;
        if(views->currentIndex()==PLAYLISTS) {utilsBar->actions().at(PLAYLISTS_UB)->setVisible(true); ui->frame_3->show();}
        resultsTable->flushTable();
        // ui->saveResults->setEnabled(false);
        ui->refreshAll->setEnabled(true);
    }

}



void MainWindow::on_settings_view_clicked()
{
    //setCoverArt(":Data/data/cover.png");
}



void MainWindow::on_rowInserted(QModelIndex model ,int x,int y)
{
    Q_UNUSED(model);Q_UNUSED(x);Q_UNUSED(y);
    qDebug()<<"indexes moved";
    addMusicImg->setVisible(false);
}

void MainWindow::on_refreshBtn_clicked()
{
    mainList->flushTable();

    clearCurrentList();
    populateMainList();
    qDebug()<<"ehat was in current list:"<<currentList;

    qDebug()<<"ehats in current list:"<<currentList;
    currentList=mainList->getAllTableContent();
    mainList->scrollToTop();
}

void MainWindow::on_tracks_view_2_clicked()
{
    expand();
}

void MainWindow::on_refreshAll_clicked()
{
    refreshTables();
}

void MainWindow::on_addAll_clicked()
{

    switch(views->currentIndex())
    {
    case COLLECTION:
        addToPlaylist( collectionTable->getAllTableContent()); break;
    case ALBUMS:
        addToPlaylist(albumsTable->albumTable->getAllTableContent()); break;
    case ARTISTS:
        addToPlaylist(artistsTable->albumTable->getAllTableContent()); break;
    case PLAYLISTS:
        addToPlaylist(playlistTable->table->getAllTableContent()); break;
    case QUEUE:
        addToPlaylist(queueTable->getAllTableContent()); break;
        //case INFO: addToPlaylist(collectionTable->getTableContent(BabeTable::LOCATION)); break;
        // case SETTINGS:  addToPlaylist(collectionTable->getTableContent(BabeTable::LOCATION)); break;
    case RESULTS:
        addToPlaylist(resultsTable->getAllTableContent()); break;

    }
}

void MainWindow::on_saveResults_clicked()
{

    qDebug()<<"on_saveResults_clicked";
    saveResults_menu->clear();

    for(auto action: collectionTable->getPlaylistMenus()) saveResults_menu->addAction(action);
    ui->saveResults->showMenu();

}



void MainWindow::saveResultsTo(QAction *action)
{
    QString playlist=action->text().replace("&","");
    switch(views->currentIndex())
    {
    case COLLECTION: collectionTable->populatePlaylist(collectionTable->getTableContent(BabeTable::LOCATION),playlist); break;
    case ALBUMS: albumsTable->albumTable->populatePlaylist(albumsTable->albumTable->getTableContent(BabeTable::LOCATION),playlist); break;
    case ARTISTS: artistsTable->albumTable->populatePlaylist(artistsTable->albumTable->getTableContent(BabeTable::LOCATION),playlist); break;
    case PLAYLISTS: playlistTable->table->populatePlaylist(playlistTable->table->getTableContent(BabeTable::LOCATION),playlist); break;
    case QUEUE: queueTable->populatePlaylist(queueTable->getTableContent(BabeTable::LOCATION),playlist); break;
        //case INFO: collectionTable->populatePlaylist(collectionTable->getTableContent(BabeTable::LOCATION),playlist); break;
        //case SETTINGS:  collectionTable->populatePlaylist(collectionTable->getTableContent(BabeTable::LOCATION),playlist); break;
    case RESULTS: resultsTable->populatePlaylist(resultsTable->getTableContent(BabeTable::LOCATION),playlist); break;

    }
}

void MainWindow::on_filterBtn_clicked()
{
    if(!showFilter)
    {
        ui->filterBtn->setChecked(true);
        ui->filterBox->setVisible(true);
        ui->filter->setFocus();
        showFilter=true;
    }else
    {
        ui->filterBtn->setChecked(false);
        ui->filterBox->setVisible(false);
        showFilter=false;
    }

}

void MainWindow::on_filter_textChanged(const QString &arg1)
{

    QString search=arg1;
    QStringList keys = {"location:","artist:","album:","title:","genre:" };
    QString key;


    for(auto k : keys)
    {

        if(search.contains(k))
        {

            key=k;
            qDebug()<<"search contains key: "<<key;
            search.replace(k,"");
        }
    }


    qDebug()<<"Searching for: "<<search;
    //int oldIndex = views->currentIndex();
    //qDebug()<<oldIndex;
    // utilsBar->actions().at(ALBUMS_UB)->setVisible(false);


    if(!search.isEmpty())
    {
        //views->setCurrentIndex(RESULTS);
        if(prevIndex==PLAYLISTS) {utilsBar->actions().at(PLAYLISTS_UB)->setVisible(false); ui->frame_3->hide();}

        mainList->flushTable();

        if(key=="location:")
        {
            mainList->populateTableView("SELECT * FROM tracks WHERE location LIKE \"%"+search+"%\"");
            //ui->search->setBackgroundRole(QPalette :: Dark);
        }else if(key== "artist:")
        {
            mainList->populateTableView("SELECT * FROM tracks WHERE artist LIKE \"%"+search+"%\"");
            //ui->search->setBackgroundRole(QPalette :: Dark);
        }else if(key== "album:")
        {
            mainList->populateTableView("SELECT * FROM tracks WHERE album LIKE \"%"+search+"%\"");
            //ui->search->setBackgroundRole(QPalette :: Dark);

        }else if(key=="title:")
        {
            mainList->populateTableView("SELECT * FROM tracks WHERE title LIKE \"%"+search+"%\"");
            ui->search->setBackgroundRole(QPalette :: Dark);
        }else if(key==  "genre:")
        {
            mainList->populateTableView("SELECT * FROM tracks WHERE genre LIKE \"%"+search+"%\"");
            //ui->search->setStyleSheet("background-color:#e3f4d7;");
            ui->search->setBackgroundRole(QPalette :: Dark);
        }else
        {

            mainList->populateTableView("SELECT * FROM tracks WHERE title LIKE \"%"+search+"%\" OR artist LIKE \"%"+search+"%\" OR album LIKE \"%"+search+"%\"OR genre LIKE \"%"+search+"%\"");
            //ui->search->setStyleSheet("background-color:transparent;");
            //ui->search->setBackgroundRole(QPalette :: Light);
        }

        //ui->saveResults->setEnabled(true);
        //ui->refreshAll->setEnabled(false);
        // prevIndex= views->currentIndex();

    }else
    {
        mainList->flushTable();
        auto old = currentList;
        currentList.clear();
        addToPlaylist(old);
    }

}
