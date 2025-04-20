import sys
import random
from PySide6 import QtCore, QtWidgets, QtGui

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
        self.label.setText(self.text[:self.start] + "<b>" + self.text[self.start:self.end] + "</b>" + self.text[self.end:])
        self.label.setTextFormat(QtCore.Qt.RichText)
        self.label.setTextInteractionFlags(QtCore.Qt.TextBrowserInteraction)

class MyWidget(QtWidgets.QWidget):
    def onedit(self, text):
        print("Text changed:", text)

    def __init__(self):
        super().__init__()
        self.layout = QtWidgets.QVBoxLayout(self)

        #test search result
        search_result = SearchResult("Hello World", 0, 5)
        self.layout.addWidget(search_result)
        search_result = SearchResult("Hello World", 0, 5)
        self.layout.addWidget(search_result)
        self.layout.addStretch()

        search_field = QtWidgets.QLineEdit()
        search_field.setPlaceholderText("Search Wikipedia")
        #print when text is changed
        search_field.textChanged.connect(self.onedit)

        self.layout.addWidget(search_field)


    @QtCore.Slot()
    def magic(self):
        self.text.setText(random.choice(self.hello))


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
        qproperty-alignment: 'AlignCenter';
    }
    '''
    )

    widget = MyWidget()
    widget.resize(800, 600)
    widget.show()

    sys.exit(app.exec())
