#include <gui/widget.h>

using namespace myos::common;
using namespace myos::gui;


Widget::Widget(Widget* parent, 
        int32_t x, int32_t y, int32_t w, int32_t h, 
        int8_t r, int8_t g, int8_t b)
{
    this->parent = parent;
    this->x = x;
    this->y = y;
    this->w = w;
    this->h = h;
    this->r = r;
    this->g = g;
    this->b = b;
    this->Foucussable = true;
}

Widget::~Widget(){
}

void Widget::GetFocus(Widget* widget){
    if(parent!=0){
        parent -> GetFocus(widget);
    }
}
void Widget::ModelToScreen(int32_t &x, int32_t &y){
    if(parent!=0){
        parent->ModelToScreen(x,y);
    }
    x += this->x;
    y += this->y;
}

void Widget::Draw(GraphicsContext* gc){
    int X = 0;
    int Y = 0;
    ModelToScreen(X,Y);
    gc->FillRectangle(X,Y,w,h,r,g,b);

}

bool Widget::ContainCoordinate(int32_t x, int32_t y){
    return this->x <= x && x < this->x +this->w && this->y <= y && this->y +this->h;
}

void Widget::OnMouseDown(int32_t x, int32_t y, uint8_t button){
    if(Foucussable){
        GetFocus(this);
    }
}

void Widget::OnMouseUp(int32_t x, int32_t y, uint8_t button){

}
void Widget::OnMouseMove(int32_t oldx, int32_t oldy, int32_t newx, int32_t newy){

}


CompositeWidget::CompositeWidget(Widget* parent, 
        int32_t x, int32_t y, int32_t w, int32_t h, 
        int8_t r, int8_t g, int8_t b)
: Widget(parent, x, y, w, h, r, g, b)
{
    focussedChild = 0;
    numChildren = 0;
}

CompositeWidget::~CompositeWidget(){

}

void CompositeWidget::GetFocus(Widget* widget){
    this->focussedChild = widget;
    if(parent != 0){
        parent->GetFocus(this);
    }
}

bool CompositeWidget::AddChild(Widget* child){
    if(numChildren >= 100){
        return false;
    }
    children[numChildren++] = child;
    return true;
}

void CompositeWidget::Draw(GraphicsContext* gc){
    Widget::Draw(gc);
    for(int i = numChildren-1; i >= 0; --i){
        children[i]->Draw(gc);
    }
}

void CompositeWidget::OnMouseDown(int32_t x, int32_t y, uint8_t button){
    for(int i = 0; i < numChildren; ++i){
        if(children[i]->ContainCoordinate(x-this->x, y-this->y))
        {
            children[i]->OnMouseDown(x-this->x, y-this->y, button);
            break;
        }
    }
}
void CompositeWidget::OnMouseUp(int32_t x, int32_t y, uint8_t button){
    for(int i = 0; i < numChildren; ++i){
        if(children[i]->ContainCoordinate(x-this->x, y-this->y))
        {
            children[i]->OnMouseUp(x-this->x, y-this->y, button);
            break;
        }
    }
}
void CompositeWidget::OnMouseMove(int32_t oldx, int32_t oldy, int32_t newx, int32_t newy){
    int firstchild = -1;
    for(int i = 0; i < numChildren; ++i){
        if(children[i]->ContainCoordinate(oldx-this->x, oldy-this->y))
        {   
            firstchild = i;
            children[i]->OnMouseMove(oldx-this->x, oldy-this->y,newx-this->x, newy-this->y);
            break;
        }
    }

    for(int i = 0; i < numChildren; ++i){
        if(children[i]->ContainCoordinate(newx-this->x, newy-this->y))
        {
            if(firstchild !=i)
                {
                    children[i]->OnMouseMove(oldx-this->x, oldy-this->y,newx-this->x, newy-this->y);
                }
            break;
        }
    }
}

void CompositeWidget::OnKeyDown(char str){
    if(focussedChild != 0){
        focussedChild->OnKeyDown(str);
    }
}
void CompositeWidget::OnKeyUp(char str){
    if(focussedChild != 0){
        focussedChild->OnKeyUp(str);
    }
}
