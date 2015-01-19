#include <stdio.h>
#include <math.h>  
#include <gsl/gsl_errno.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_odeiv2.h>            
#include <cairo.h>  
#include <gtk/gtk.h>

double globx;

double analytic(double w0, double t);

double spring_euler(double w0, double x);
void test_euler(GtkWidget *darea);

int func(double t, const double x[], double f[], void *params);
int jac(double t, const double x[], double *dfdy, double dfdt[], void *params);
int test_gsl(GtkWidget *darea);

void do_drawing(cairo_t *cr);
gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data);

gboolean terminate(GThread *thread);

void *thread_func(GtkWidget *entry) {
  //test_euler(entry);
  test_gsl(entry);

  gdk_threads_add_idle((GSourceFunc)terminate, g_thread_self());
  return NULL;
}

int main(int argc, char *argv[]) {
	GtkWidget *window;
	GtkWidget *darea;

    gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	darea = gtk_drawing_area_new();
	gtk_container_add(GTK_CONTAINER(window), darea);

	g_signal_connect(G_OBJECT(darea), "draw", G_CALLBACK(on_draw_event), NULL); 
	g_signal_connect(window, "destroy",	G_CALLBACK(gtk_main_quit), NULL);  
	 
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 400, 300); 
	gtk_window_set_title(GTK_WINDOW(window), "Spring simulation");

	gtk_widget_show_all(window);

  	g_thread_new("eval", (GThreadFunc)thread_func, darea);

	gtk_main();

	return 0;	
}

double analytic(double w0, double t) {
	return cos(w0 * t);
}

// euler

double spring_euler(double w0, double x) {
	return -w0 * w0 * x;
} 

void test_euler(GtkWidget *darea) {
	const int n = 1000;

	// x - position
	// v - speed
	// t - time
	// dt - step
	// w0 = sqrt(k/m)
	double x[n], v[n], t[n], dt;
	
	const double w0 = 1.0;
	x[0] = 1.0; v[0] = 0.0; t[0] = 0.0;
	dt = 0.05;                             

	int i;
	for (i = 0; i < n - 1 ; i++) {
		x[i+1] = x[i] + v[i] * dt;
		v[i+1] = v[i] + spring_euler(w0, x[i]) * dt;
		t[i+1] = t[i] + dt;
	}

	for (i = 0; i < n; i++) {
		// draw
		globx = x[i];
		gtk_widget_queue_draw(darea);
		printf("t: %f\tx: %f\tanalytic: \t%f\n", t[i], x[i], analytic(w0, t[i])); 
		g_usleep(10000);  
	}
}

// gsl

int func(double t, const double x[], double f[], void *params) {
	// x' = v
	// v' = -w0^2 * x
	double w0 = *(double *) params;
	f[0] = x[1];
	f[1] = -w0 * w0 * x[0];
	return GSL_SUCCESS;
}

int jac(double t, const double x[], double *dfdy, double dfdt[], void *params) {
	double w0 = *(double *)params;
	gsl_matrix_view dfdy_mat = gsl_matrix_view_array(dfdy, 2, 2);
	gsl_matrix * m = &dfdy_mat.matrix; 
	gsl_matrix_set(m, 0, 0, 0.0);
	gsl_matrix_set(m, 0, 1, 1.0);
	gsl_matrix_set(m, 1, 0, -w0 * w0);
	gsl_matrix_set(m, 1, 1, 0.0);
	dfdt[0] = 0.0;
	dfdt[1] = 0.0;
	return GSL_SUCCESS;
}

int test_gsl(GtkWidget *darea) {
	double w0 = 1.0;
	gsl_odeiv2_system sys = { func, jac, 2, &w0 };

	gsl_odeiv2_driver *d = gsl_odeiv2_driver_alloc_y_new(&sys, gsl_odeiv2_step_rk4, 0.05, 1e-6, 1e-6);

	double t = 0.0;
	double x[2] = { 1.0, 0.0 };
	int i, s;

	for (i = 0; i < 1000; i++) {
		s = gsl_odeiv2_driver_apply_fixed_step(d, &t, 0.05, 1, x);

		if (s != GSL_SUCCESS) {
			printf ("error: driver returned %d\n", s);
			break;
		}

		// draw
		globx = x[0];
		gtk_widget_queue_draw(darea);
		printf("t: %.4f\tanalytic: %.8f\tx:%.8f\n", t, analytic(w0, t), x[0]);
		g_usleep(10000); 
	}

	gsl_odeiv2_driver_free(d);
	return s;
}

// drawing

gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
	do_drawing(cr);
	
	return FALSE;
}

void do_drawing(cairo_t *cr) {
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_set_line_width(cr, 1);

	int k;
	int prevy = 0;
	cairo_move_to(cr, 200, 0);
	for (k = 0; k < 20; k++) {
		int j = k % 2 == 0 ? -1 : 1;
		cairo_line_to(cr, 200 + j * 20, k*5*(globx + 1) + prevy);
		prevy = k*2;
	}
	cairo_line_to(cr, 200, 100*(globx + 1) + prevy);
	//cairo_stroke(cr);    
	cairo_rectangle(cr, 180, 100*(globx + 1) + prevy, 40, 40);
	//cairo_stroke_preserve(cr);    
	//cairo_fill(cr);
	cairo_stroke(cr);
}

// threads

gboolean terminate(GThread *thread) {
  g_thread_join(thread);

  //g_thread_unref(thread);

  return FALSE;
}