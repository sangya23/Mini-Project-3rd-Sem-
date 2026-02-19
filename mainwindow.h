#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QAudioOutput>
#include <QTableWidgetItem>
#include <QListWidgetItem>
#include <QMap>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <stack>

struct Song {
    QString title;
    QString artist;
    QString filePath;
    QString mood;
    bool isFavorite;
    Song* next = nullptr;
    Song* prev = nullptr;
    Song(QString t, QString a, QString p, QString m)
        : title(t), artist(a), filePath(p), mood(m), isFavorite(false) {}
};


struct BSTNode {
    QString title;
    Song* songData;
    BSTNode *left, *right;
    BSTNode(Song* s) : title(s->title), songData(s), left(nullptr), right(nullptr) {}
};

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updatePosition(qint64 position);
    void updateDuration(qint64 duration);
    void on_playbutton_clicked();
    void on_Addsongbutton_clicked();
    void on_prevbutton_clicked();
    void on_nextbutton_clicked();
    void on_searchlineedit_textChanged(const QString &arg1);
    void on_listWidget_itemClicked(QListWidgetItem *item);
    void on_Sortcombobox_currentIndexChanged(int index);
    void on_table_item_clicked(int row, int column);
    void on_favButton_clicked();
    void on_deleteSong_clicked();
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);


    void on_backHistoryButton_clicked();
    void on_clearhistorybutton_clicked();

private:
    Ui::MainWindow *ui;

    QMediaPlayer *player;
    QAudioOutput *audioOutput;

    Song* head = nullptr;
    Song* tail = nullptr;
    Song* currentPlayingSong = nullptr;

    std::stack<Song*> historyStack;

    BSTNode* bstRoot = nullptr;


    Song* mergeSort(Song* headSource, int sortIndex);
    Song* sortedMerge(Song* a, Song* b, int sortIndex);
    void splitList(Song* source, Song** frontRef, Song** backRef);


    BSTNode* insertBST(BSTNode* root, Song* s);
    void rebuildSearchTree();
    void clearBST(BSTNode* root);
    Song* searchInBST(BSTNode* root, QString title);

    void addSongToTable(Song* s);
    void refreshTable();
    void updateFavButtonUI();
    void deleteSong(Song* toDelete);
    void loadLibrary();
    void updateLibraryFile();
    void updateHistoryUI();

    void playSelectedSong(Song* s, bool recordHistory = true);
};

#endif // MAINWINDOW_H
