#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    player = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    player->setAudioOutput(audioOutput);
    audioOutput->setVolume(0.5);


    ui->tableWidget->setColumnCount(3);
    QStringList headers;
    headers << "Song Title" << "Artist" << "Mood";
    ui->tableWidget->setHorizontalHeaderLabels(headers);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    ui->listWidget->addItem("All Songs");
    ui->listWidget->addItem("Happy");
    ui->listWidget->addItem("Chill");
    ui->listWidget->addItem("Workout");
    ui->listWidget->addItem("Sad");
    ui->listWidget->addItem("❤ Favorites");

    connect(ui->tableWidget, &QTableWidget::cellClicked, this, &MainWindow::on_table_item_clicked);
    connect(player, &QMediaPlayer::positionChanged, this, &MainWindow::updatePosition);
    connect(player, &QMediaPlayer::durationChanged, this, &MainWindow::updateDuration);
    connect(player, &QMediaPlayer::mediaStatusChanged, this, &MainWindow::onMediaStatusChanged);
    loadLibrary();
    rebuildSearchTree();
}

MainWindow::~MainWindow() {
    clearBST(bstRoot);
    delete ui;
}
void MainWindow::on_Addsongbutton_clicked() {
    QString filePath = QFileDialog::getOpenFileName(this, "Select Audio", "", "MP3 Files (*.mp3)");
    if (!filePath.isEmpty()) {
        Song* temp = head;
        while (temp) {
            if (temp->filePath == filePath) {
                QMessageBox::warning(this, "Duplicate Song", "This song is already in your library!");
                return;
            }
            temp = temp->next;
        }
        QString title = filePath.split("/").last();
        bool okMood, okArtist;
        QStringList moods = {"Happy", "Chill", "Workout", "Sad"};
        QString selectedMood = QInputDialog::getItem(this, "Select Mood", "Mood:", moods, 0, false, &okMood);
        QString selectedArtist = QInputDialog::getText(this, "Artist Info", "Artist Name:", QLineEdit::Normal, "Unknown Artist", &okArtist);

        if (okMood && okArtist) {
            Song* newSong = new Song(title, selectedArtist, filePath, selectedMood);
            if (!head) { head = tail = newSong; }
            else { tail->next = newSong; newSong->prev = tail; tail = newSong; }

            addSongToTable(newSong);
            rebuildSearchTree();
            updateLibraryFile();
        }
    }
}

void MainWindow::on_listWidget_itemClicked(QListWidgetItem *item) {
    QString selectedMood = item->text();
    ui->tableWidget->setRowCount(0);

    Song* temp = head;
    while (temp) {
        if (selectedMood == "All Songs") addSongToTable(temp);
        else if (selectedMood == "❤ Favorites" && temp->isFavorite) addSongToTable(temp);
        else if (temp->mood == selectedMood) addSongToTable(temp);
        temp = temp->next;
    }
}

void MainWindow::on_favButton_clicked() {
    if (currentPlayingSong) {
        currentPlayingSong->isFavorite = !currentPlayingSong->isFavorite;
        updateFavButtonUI();
        updateLibraryFile();
    }
}

void MainWindow::on_deleteSong_clicked() {
    if (!currentPlayingSong) return;
    if (QMessageBox::question(this, "Delete", "Delete '" + currentPlayingSong->title + "'?") == QMessageBox::Yes) {
        deleteSong(currentPlayingSong);
        updateLibraryFile();
        rebuildSearchTree();
    }
}

void MainWindow::deleteSong(Song* toDelete) {
    if (!toDelete) return;


    if (toDelete->prev) toDelete->prev->next = toDelete->next;
    if (toDelete->next) toDelete->next->prev = toDelete->prev;

    if (toDelete == head) head = toDelete->next;
    if (toDelete == tail) tail = toDelete->prev;


    if (currentPlayingSong == toDelete) {
        player->stop();
        currentPlayingSong = nullptr;
        ui->currentsonglabel->setText("Song Deleted");
    }


    while(!historyStack.empty()) historyStack.pop();
    ui->listWidget_History->clear();

    delete toDelete;

    refreshTable();
    rebuildSearchTree();
}

void MainWindow::on_backHistoryButton_clicked() {
    if (!historyStack.empty()) {
        Song* lastPlayed = historyStack.top();
        historyStack.pop();
        playSelectedSong(lastPlayed, false);
    } else {
        QMessageBox::information(this, "History", "No more songs in history stack.");
    }
}



void MainWindow::onMediaStatusChanged(QMediaPlayer::MediaStatus status) {
    if (status == QMediaPlayer::EndOfMedia) {
        on_nextbutton_clicked();
    }
}

void MainWindow::on_searchlineedit_textChanged(const QString &arg1) {
    ui->tableWidget->setRowCount(0);
    if (arg1.isEmpty()) {
        refreshTable();
        return;
    }

    Song* temp = head;
    while (temp) {
        if (temp->title.toLower().contains(arg1.toLower())) {
            addSongToTable(temp);
        }
        temp = temp->next;
    }
}

BSTNode* MainWindow::insertBST(BSTNode* root, Song* s) {
    if (!root) return new BSTNode(s);
    if (s->title.toLower() < root->title.toLower())
        root->left = insertBST(root->left, s);
    else
        root->right = insertBST(root->right, s);
    return root;
}

Song* MainWindow::searchInBST(BSTNode* root, QString title) {
    if (!root) return nullptr;
    if (root->title.toLower().startsWith(title)) return root->songData;
    if (title < root->title.toLower()) return searchInBST(root->left, title);
    return searchInBST(root->right, title);
}

void MainWindow::rebuildSearchTree() {
    clearBST(bstRoot);
    bstRoot = nullptr;
    Song* temp = head;
    while (temp) {
        bstRoot = insertBST(bstRoot, temp);
        temp = temp->next;
    }
}

void MainWindow::clearBST(BSTNode* root) {
    if (!root) return;
    clearBST(root->left);
    clearBST(root->right);
    delete root;
}



void MainWindow::on_Sortcombobox_currentIndexChanged(int index) {
    if (!head || !head->next) return;
    head = mergeSort(head, index);

    Song* temp = head;
    while (temp && temp->next) temp = temp->next;
    tail = temp;

    refreshTable();
}

Song* MainWindow::mergeSort(Song* headSource, int sortIndex) {
    if (!headSource || !headSource->next) return headSource;
    Song *a, *b;
    splitList(headSource, &a, &b);
    a = mergeSort(a, sortIndex);
    b = mergeSort(b, sortIndex);
    return sortedMerge(a, b, sortIndex);
}

void MainWindow::splitList(Song* source, Song** frontRef, Song** backRef) {
    Song* fast = source->next;
    Song* slow = source;
    while (fast != nullptr) {
        fast = fast->next;
        if (fast != nullptr) {
            slow = slow->next;
            fast = fast->next;
        }
    }
    *frontRef = source;
    *backRef = slow->next;
    slow->next = nullptr;
    if (*backRef) (*backRef)->prev = nullptr;
}

Song* MainWindow::sortedMerge(Song* a, Song* b, int sortIndex) {
    if (!a) return b;
    if (!b) return a;
    bool compare = (sortIndex == 0) ? (a->title.toLower() <= b->title.toLower()) : (a->artist.toLower() <= b->artist.toLower());
    Song* result;
    if (compare) {
        result = a;
        result->next = sortedMerge(a->next, b, sortIndex);
        if (result->next) result->next->prev = result;
    } else {
        result = b;
        result->next = sortedMerge(a, b->next, sortIndex);
        if (result->next) result->next->prev = result;
    }
    return result;
}

void MainWindow::playSelectedSong(Song* s, bool recordHistory) {
    if (!s) return;

    if (recordHistory && currentPlayingSong && currentPlayingSong != s) {
        historyStack.push(currentPlayingSong);
        updateHistoryUI();
    }

    currentPlayingSong = s;
    player->setSource(QUrl::fromLocalFile(s->filePath));
    player->play();

    for (int i = 0; i < ui->tableWidget->rowCount(); ++i) {
        if (ui->tableWidget->item(i, 0)->text() == s->title) {
            ui->tableWidget->setCurrentCell(i, 0);
            break;
        }
    }

    ui->currentsonglabel->setText(s->title);
    ui->playbutton->setText("Pause");
    updateFavButtonUI();
}

void MainWindow::on_playbutton_clicked() {
    if (player->playbackState() == QMediaPlayer::PlayingState) {
        player->pause();
        ui->playbutton->setText("Play");
    } else {
        if (!currentPlayingSong && head) playSelectedSong(head);
        else player->play();
        ui->playbutton->setText("Pause");
    }
}

void MainWindow::on_nextbutton_clicked() {
    int currentRow = ui->tableWidget->currentRow();
    if (currentRow != -1 && currentRow < ui->tableWidget->rowCount() - 1) {
        ui->tableWidget->setCurrentCell(currentRow + 1, 0);
        on_table_item_clicked(currentRow + 1, 0);
    } else if (currentPlayingSong && currentPlayingSong->next) {
        playSelectedSong(currentPlayingSong->next);
    }
}

void MainWindow::on_prevbutton_clicked() {
    if (currentPlayingSong && currentPlayingSong->prev) playSelectedSong(currentPlayingSong->prev);
}

void MainWindow::addSongToTable(Song* s) {
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);
    ui->tableWidget->setItem(row, 0, new QTableWidgetItem(s->title));
    ui->tableWidget->setItem(row, 1, new QTableWidgetItem(s->artist));
    ui->tableWidget->setItem(row, 2, new QTableWidgetItem(s->mood));
}

void MainWindow::refreshTable() {
    ui->tableWidget->setRowCount(0);
    Song* temp = head;
    while (temp) {
        addSongToTable(temp);
        temp = temp->next;
    }
}

void MainWindow::loadLibrary() {
    QFile file("library.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QStringList d = in.readLine().split("|");
        if (d.size() >= 4) {
            Song* s = new Song(d[0], d[1], d[2], d[3]);
            if (d.size() == 5) s->isFavorite = (d[4] == "1");
            if (!head) head = tail = s;
            else { tail->next = s; s->prev = tail; tail = s; }
            addSongToTable(s);
        }
    }
    file.close();
}

void MainWindow::updateLibraryFile() {
    QFile file("library.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        Song* t = head;
        while (t) {
            out << t->title << "|" << t->artist << "|" << t->filePath << "|" << t->mood << "|" << (t->isFavorite ? "1" : "0") << "\n";
            t = t->next;
        }
    }
}

void MainWindow::updatePosition(qint64 pos) { ui->horizontalSlider->setValue(pos); }
void MainWindow::updateDuration(qint64 dur) { ui->horizontalSlider->setRange(0, dur); }
void MainWindow::on_table_item_clicked(int r, int c) {
    QString title = ui->tableWidget->item(r, 0)->text();
    Song* t = head;
    while(t) { if(t->title == title) { playSelectedSong(t); break; } t = t->next; }
}
void MainWindow::updateFavButtonUI() {
    if (!currentPlayingSong) return;
    ui->favButton->setText(currentPlayingSong->isFavorite ? "❤" : "♡");

}

void MainWindow::updateHistoryUI() {
    ui->listWidget_History->clear();

    std::stack<Song*> tempStack = historyStack;

    while (!tempStack.empty()) {
        Song* s = tempStack.top();
        ui->listWidget_History->addItem(s->title);
        tempStack.pop();
    }
}

void MainWindow::on_clearhistorybutton_clicked() {
    while(!historyStack.empty()) historyStack.pop();
    ui->listWidget_History->clear();
}
