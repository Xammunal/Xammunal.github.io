
class RectangleElement {
protected:
    // Размер элемента игрового поля (прямоугольника)
    int sizeX;
    int sizeY;

public:
    // Конструктор класса
    RectangleElement() {
        // Инициализируем размер элемента (прямоугольника)
        sizeX = 99;
        sizeY = 30;
    }

    int getSizeX() const { return sizeX; }
    int getSizeY() const { return sizeY; }
};