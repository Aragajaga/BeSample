#include <iostream>

#include <GLView.h>
#include <InterfaceKit.h>
#include <DirectWindow.h>
#include <Window.h>
#include <Application.h>

class SampleView : public BGLView
{
public:
			SampleView(BRect rect, const char *name, ulong resizingMode, ulong options);
			~SampleView();
	virtual void	AttachedToWindow();
	virtual void	DetachedFromWindow();
	virtual void 	DrawFrame(bool noPause);

	sem_id		m_drawEvent;
	sem_id		m_quittingSem;
private:
	thread_id	m_drawThread;
};

class SampleWindow : public BDirectWindow {
public:
			SampleWindow(BRect r, const char *name, window_type wt, ulong something);
	virtual bool 	QuitRequested();
	virtual void	DirectConnected(direct_buffer_info *info);
	virtual void	MessageReceived(BMessage *msg);
private:
	SampleView	*m_view;
};

class SampleApplication : public BApplication {
public:
			SampleApplication();
	virtual void	MessageReceived(BMessage *msg);
private:
	SampleWindow	*m_window;
};

// classes declarations end

static int32 summonThread(void *cookie)
{
	SampleView *view = reinterpret_cast<SampleView *>(cookie);
	BScreen screen(view->Window());

	while (acquire_sem_etc(view->m_quittingSem, 1, B_TIMEOUT, 0) == B_NO_ERROR)
	{
		view->DrawFrame(0);
		release_sem(view->m_quittingSem);
		screen.WaitForRetrace();
	}
	return 0;
}

SampleView::SampleView(BRect rect, const char *name, ulong resizingMode, ulong options)
: BGLView(rect, name, resizingMode, 0, options)
{
	m_quittingSem = create_sem(1, "quitting sem");
	m_drawEvent = create_sem(0, "draw event");
}

SampleView::~SampleView()
{
	delete_sem(m_quittingSem);
	delete_sem(m_drawEvent);
}

void SampleView::AttachedToWindow()
{
	BGLView::AttachedToWindow();

	m_drawThread = spawn_thread(summonThread, "summonThread", B_NORMAL_PRIORITY, this);
	resume_thread(m_drawThread);
}

void SampleView::DetachedFromWindow()
{
	BGLView::DetachedFromWindow();

	long locks= 0;
	status_t dummy;

	while (Window()->IsLocked())
	{
		locks++;
		Window()->Unlock();
	}

	acquire_sem(m_quittingSem);
	release_sem(m_drawEvent);
	wait_for_thread(m_drawThread, &dummy);
	release_sem(m_quittingSem);

	while (locks--)
		Window()->Lock();
}

void SampleView::DrawFrame(bool noPause)
{
	std::cout << "DrawFrame" << std::endl;
	LockGL();
	glClearColor(0.f, 1.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();


	glPopMatrix();
	SwapBuffers();
	UnlockGL();
}

SampleWindow::SampleWindow(BRect rect, const char *name, window_type wt, ulong something)
: BDirectWindow(rect, name, wt, something)
{
	Lock();
	GLenum type = BGL_RGB | BGL_DEPTH | BGL_DOUBLE;
	BRect bounds = Bounds();
	m_view = new(std::nothrow) SampleView(bounds, "m_view", B_FOLLOW_ALL_SIDES, type);
	AddChild(m_view);
	Unlock();
}

bool SampleWindow::QuitRequested()
{
	if (m_view != nullptr)
		m_view->EnableDirectMode(false);

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void SampleWindow::DirectConnected(direct_buffer_info *info)
{
	if (m_view != nullptr) {
		m_view->DirectConnected(info);
		m_view->EnableDirectMode(true);
	}
}

void SampleWindow::MessageReceived(BMessage *msg)
{
	msg->PrintToStream();
	switch (msg->what)
	{
	default:
		BDirectWindow::MessageReceived(msg);
	}
}

SampleApplication::SampleApplication()
: BApplication("application/x-vnd.Aragajaga.SampleApplication")
{
	m_window = new SampleWindow(BRect(64, 64, 640, 360), "SampleApplication", B_TITLED_WINDOW, 0);
	m_window->Show();
}

void SampleApplication::MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
	default:
		BApplication::MessageReceived(msg);
	}
}

int main()
{
	try {
		SampleApplication *app = new SampleApplication();
		app->Run();
		delete app;
	}
	catch (...) {
		std::cerr << "An error occured." << '\n';
	}
	return 0;
}

