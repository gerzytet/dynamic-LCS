import sys
import random
from PySide6 import QtCore, QtWidgets, QtGui
import socket

#single line search result, with matched text highlighted
class SearchResult(QtWidgets.QWidget):
    def __init__(self, text, start, end):
        super().__init__()
        self.text = text
        self.start = start
        self.end = end

        self.layout = QtWidgets.QHBoxLayout(self)
        self.label = QtWidgets.QLabel(self.text)
        self.layout.setContentsMargins(0, 0, 0, 0)
        self.layout.setSpacing(0)

        self.layout.addWidget(self.label)

        #highlight the matched text
        self.label.setText(self.text[:self.start] + "<b>" + self.text[self.start:self.end+1] + "</b>" + self.text[self.end+1:])
        self.label.setTextFormat(QtCore.Qt.RichText)
        self.label.setTextInteractionFlags(QtCore.Qt.TextBrowserInteraction)

def calc_diff(s, t):
    i = 0
    diff = ""
    while i < len(s) and i < len(t):
        if (s[i] != t[i]):
            diff += "r" + str(i) + t[i]
        i += 1
    if (i < len(t)):
        diff += ''.join(['+' + c for c in t[i:]])
    if (i < len(s)):
        diff += "-"*len(s[i:])
    return diff

def get_reply():
    data = ""
    while '\n\n' not in data:
        data += s.recv(1024).decode()
    return data

def search_results_from_reply(reply):
    ans = []
    lines = reply.split('\n')
    lines = lines[1:-2]
    for i in range(0, len(lines), 3):
        title = lines[i]
        start = int(lines[i+1])
        end = int(lines[i+2])
        ans.append(SearchResult(title, start, end))
    return ans

class MyWidget(QtWidgets.QWidget):

    def onedit(self, text):
        diff = calc_diff(self.prev_text, text)
        self.prev_text = text
        print("diff:", diff)
        s.sendall((diff + '\n').encode())
        reply = get_reply()
        results = search_results_from_reply(reply)
        for result in self.results:
            result.deleteLater()
        self.results = []
        i = 1
        for result in results:
            self.layout.insertWidget(i, result)
            self.results.append(result)
            i += 1

    def __init__(self):
        print("init")
        self.prev_text = ""
        super().__init__()
        self.layout = QtWidgets.QVBoxLayout(self)
        self.results = []

        top_text = QtWidgets.QLabel("Results: ")
        self.layout.addWidget(top_text)

        #test search result
        self.layout.addStretch()

        search_field = QtWidgets.QLineEdit()
        search_field.setPlaceholderText("Search Wikipedia")
        #print when text is changed
        search_field.textChanged.connect(self.onedit)

        self.layout.addWidget(search_field)


s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(('127.0.0.1', 19921))

if __name__ == "__main__":
    app = QtWidgets.QApplication([])
    app.setStyleSheet(
    '''
    QLabel {
        font-size: 16px;
        qproperty-alignment: 'AlignLeft';
    }

    QLineEdit {
        font-size: 16px;
        min-height: 30px;
        qproperty-alignment: 'AlignLeft';
    }
    '''
    )

    widget = MyWidget()
    widget.resize(800, 600)
    widget.show()

    sys.exit(app.exec())
