//////	DETECCIÓN DE PLAZAS DE APARCAMIENTO	//////

#include "functions.h"

#define camara 1
#define dimensionesROI 15
#define umbralPresencia 200

/* DECLARACIÓN DE FUNCIONES GLOBALES */
void onMouse(int event, int x, int y, int, void*);

/* VARIABLES GLOBALES DEL PROGRAMA */

// Puntos donde se almacenan las coordenadas del pixel en el cual estamos haciendo click
Point pulsar, soltar;
// Plaza donde estará aparcado el coche
vector<Rect> plaza;
Rect seleccion;
// Variable que informa al bucle principal que ya hemos seleccionado todas las plazas
Mat frame;
bool plazaSeleccionada = false;
int numeroPlazas = 1;
int plazasSeleccionadas = 0;

int main()
{
	/* INICIALIZACIÓN DEL PROGRAMA Y ACCESO A LA CÁMARA */

	Mat frame_gray, umbral(frame.size(), CV_8U);
	char esc;

	std::cout << endl << "Inacializacion del programa. Accediendo a la camara...." << endl;
	// Capturamos las imágenes de la cámara del ordenador
	VideoCapture cam(camara);
	// Comprobamos que no existan errores
	if ( !cam.isOpened() )
	{
		std::cout << "Error al acceder a la camara del ordenador." << endl;
		getchar();
		return(EXIT_FAILURE);
	}

	// Selección del número de plazas
	cout << endl << "Introduzca el numero de plazas: ";
	cin >> numeroPlazas;

	plaza.resize(numeroPlazas);

	// Ventanas para mostrar los resultados
	namedWindow("camara", 1);
	namedWindow("bordes", 1);
	namedWindow("umbral", 1);
	namedWindow("plazas", 1);

	// Activamos los eventos del ratón
	setMouseCallback("camara", onMouse, 0);

	/* SELECCIÓN DE LAS PLANTAS DE APARCAMIENTO */

	Mat salidaBordes(Size(frame.cols, frame.rows), CV_8U);
	Mat drawing;
	RNG rng(12345);
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;

	cout << endl << "Seleccion de las plazas del aparcamiento." << endl;
	cout << "Pulse la tecla esc si desea cancelar la operacion y salir del programa." << endl << endl;
	while (1)
	{
		cam >> frame;

		// Ha ocurrido un error y no hemos podido acceder al la cámara, salimos
		if (!frame.data)
		{
			cout << "Error capturando frames de la cámara" << endl;
			getchar();
			exit(-1);
		}

		// Eliminación de ruido
		GaussianBlur(frame, frame, Size(3, 3), 0, 0);

		// Realce de bordes
		//frame = realceDeBordes(frame);

		// Umbralizamos el parking para que el algoritmo de detección de bordes funcione mejor
		cvtColor(frame, frame_gray, CV_BGR2GRAY);
		threshold(frame_gray, umbral, 127, 255, CV_THRESH_BINARY_INV);

		// Aplicamos el algoritmo de Canny para la deteccción de bordes
		Canny(frame_gray, salidaBordes, 40, 40*3);
		//dilate(salidaBordes, salidaBordes, Mat(), Point(-1,-1), 3);
		findContours(salidaBordes, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_NONE, Point(0, 0));

		// Buscamos los rectángulos que forman las plazas
		vector<RotatedRect> minRect(contours.size());

		for (unsigned int i = 0; i < contours.size(); i++)
		{
			minRect[i] = minAreaRect(Mat(contours[i]));
		}

		// Dibujamos los contornos y los rectángulos que forman las plazas
		drawing = Mat::zeros(frame.size(), CV_8UC3);
		for (unsigned int i = 0; i < contours.size(); i++)
		{
			Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
			// contour
			drawContours(drawing, contours, i, color, 1, 8, hierarchy, 0, Point());
			// rotated rectangle
			Point2f rect_points[4]; minRect[i].points(rect_points);
			for (int j = 0; j < 4; j++)
				line(drawing, rect_points[j], rect_points[(j + 1) % 4], color, 1, 8);
		}

		// Mostramos los resultados durante la elección de la plaza
		imshow("camara", frame);
		imshow("umbral", umbral);
		imshow("bordes", salidaBordes);
		imshow("plazas", drawing);

		// Podemos salir del bucle pulsando cualquier tecla manualmente o cuando haya finalizado la selección de plazas sale 
		// automáticamente. Para salir del programa hay que pulsar la tecla esc
		esc = cv::waitKey(30);
		if ( numeroPlazas == plazasSeleccionadas )
			break;
		if (esc == 27)
			return 0;
	}
	// Una vez seleccionada la plaza procedemos a eliminar las ventanas anteriormente abiertas
	destroyAllWindows();

	/* CREACIÓN DE LAS REGIONES DE INTERÉS (ROIs) */
	vector<Rect> ROIs1;
	vector<Rect> ROIs2;

	// Antes de nada es necesario comprobar si cada plaza está en horizontal o vertical respecto a la cámara
	for (int i = 0; i < numeroPlazas; i++)
	{
		if (plaza[i].width <= plaza[i].height)
		{
			// Si el rectángulo tiene más de 10x20 píxeles podemos crear dentro dos rectángulos de 15x15
			if (plaza[i].width > 20 && plaza[i].height > 40)
			{
				int x0, x2, y2, y0;
				// Calculamos las posiciones de las ROIs y sus dimensiones
				x2 = plaza[i].x + plaza[i].width;
				y2 = plaza[i].y + plaza[i].height;
				x0 = cvRound(plaza[i].x + (x2 - plaza[i].x) / 2 - (dimensionesROI / 2));
				y0 = cvRound(plaza[i].y + (y2 - plaza[i].y) / 4 - (dimensionesROI / 2));
				// Calculamos ROI1 y ROI2
				ROIs1.push_back(Rect(x0, y0, dimensionesROI, dimensionesROI));
				y0 = cvRound(plaza[i].y + 3 * (y2 - plaza[i].y) / 4 - (dimensionesROI / 2));
				ROIs2.push_back(Rect(x0, y0, dimensionesROI, dimensionesROI));
			}
			else
			{
				// Si el tamaño de la plaza es demasiado pequeño, el algoritmo no va a funcionar bien, por lo que se desecha
				if ((plaza[i].width < dimensionesROI) || (plaza[i].height < dimensionesROI))
				{
					cout << "La plaza seleccionada es demasiado pequeña para el correcto funcionamiento del algoritmo." << endl;
					cout << "Fin del programa" << endl;
					getchar();
					exit(EXIT_FAILURE);
				}
				// Solo tendríamos en este caso una ROI en el centro de la plaza
				int x0, y0;
				x0 = cvRound(plaza[i].x + plaza[i].width / 2 - dimensionesROI / 2);
				y0 = cvRound(plaza[i].y + plaza[i].height / 2 - dimensionesROI / 2);
				ROIs1.push_back(Rect(x0, y0, dimensionesROI, dimensionesROI));
			}
		}
		else
			// La plaza está horizontal respesto a la cámara. El código será igual que el anterior, pero modificando la anchura por altura
		{
			{
				// Si el rectángulo tiene más de 10x20 píxeles podemos crear dentro dos rectángulos de 15x15
				if (plaza[i].height > 20 && plaza[i].width > 40)
				{
					int x0, x2, y2, y0;
					// Calculamos las posiciones de las ROIs y sus dimensiones
					x2 = plaza[i].x + plaza[i].width;
					y2 = plaza[i].y + plaza[i].height;
					x0 = cvRound(plaza[i].x + (x2 - plaza[i].x) / 4 - (dimensionesROI / 2));
					y0 = cvRound(plaza[i].y + (y2 - plaza[i].y) / 2 - (dimensionesROI / 2));
					// Calculamos ROI1 y ROI2
					ROIs1.push_back(Rect(x0, y0, dimensionesROI, dimensionesROI));
					x0 = cvRound(plaza[i].x + 3 * (x2 - plaza[i].x) / 4 - (dimensionesROI / 2));
					ROIs2.push_back(Rect(x0, y0, dimensionesROI, dimensionesROI));
				}
				else
				{
					// Si el tamaño de la plaza es demasiado pequeño, el algoritmo no va a funcionar bien, por lo que se desecha
					if ((plaza[i].width < dimensionesROI) || (plaza[i].height < dimensionesROI))
					{
						cout << "La plaza seleccionada es demasiado pequeña para el correcto funcionamiento del algoritmo." << endl;
						cout << "Fin del programa" << endl;
						getchar();
						exit(EXIT_FAILURE);
					}
					// Solo tendríamos en este caso una ROI en el centro de la plaza
					int x0, y0;
					x0 = cvRound(plaza[i].x + plaza[i].width / 2 - dimensionesROI / 2);
					y0 = cvRound(plaza[i].y + plaza[i].height / 2 - dimensionesROI / 2);
					ROIs1.push_back(Rect(x0, y0, dimensionesROI, dimensionesROI));
				}
			}
		}
	}

	// Guardamos las imágenes de los ROIs cuando el parking está vacío
	vector<Mat> fondo_ROI1;
	vector<Mat> fondo_ROI2;
	for (int i = 0; i < numeroPlazas; i++)
	{
		fondo_ROI1.push_back(Mat(frame_gray, ROIs1[i]));
		medianBlur(fondo_ROI1[i], fondo_ROI1[i], 3);
		fondo_ROI2.push_back(Mat(frame_gray, ROIs2[i]));
		medianBlur(fondo_ROI2[i], fondo_ROI2[i], 3);
	}
	
	/* VIGILANCIA DEL APARCAMIENTO */

	vector<Mat> imagen_ROI1, imagen_ROI2;
	Mat frame_vigilancia, frame_vigilancia_gray;
	double norm1, norm2;
	bool plazaOcupada = false;
	Scalar rojo = Scalar(0, 0, 255);
	Scalar verde = Scalar(0, 255, 0);
	Scalar mean, stddev;
	namedWindow("Vigilancia", 1);

	cout << endl << "Plazas seleccionadas, procediendo a la vigilancia del aparcamiento..." << endl << endl;
	while (1)
	{
		cam >> frame_vigilancia;

		// Si ha ocurrido un error y no hemos podido acceder al la cámara, salimos
		if (!frame_vigilancia.data)
		{
			cout << "Error capturando frames de la cámara" << endl;
			getchar();
			exit(EXIT_FAILURE);
		}

		// Eliminación de ruido y conversión a escala de grises
		GaussianBlur(frame_vigilancia, frame_vigilancia, Size(3, 3), 0, 0);
		cvtColor(frame_vigilancia, frame_vigilancia_gray, CV_BGR2GRAY);

		for (int i = 0; i < numeroPlazas; i++)
		{
			// Para cada ROI actualizamos las imágenes y calculamos la "norma relativa diferencial"
			imagen_ROI1.push_back(Mat(frame_vigilancia_gray, ROIs1[i]));
			//medianBlur(imagen_ROI1, imagen_ROI1, 3);
			imagen_ROI2.push_back(Mat(frame_vigilancia_gray, ROIs2[i]));
			//medianBlur(imagen_ROI2, imagen_ROI2, 3);

			norm1 = norm(fondo_ROI1[i], imagen_ROI1[i], NORM_L2);
			cout << "Norma 1: " << norm1 << endl;
			norm2 = norm(fondo_ROI2[i], imagen_ROI2[i], NORM_L2);
			cout << "Norma 2: " << norm2 << endl;

			// Si una de estas normas supera el umbral, asumimos la presencia de un coche
			if ((norm1 > umbralPresencia) || (norm2 > umbralPresencia))
			{
				cout << "Plaza ocupada" << endl;
				plazaOcupada = true;
			}
			else
			{
				cout << "Plaza libre" << endl;
				plazaOcupada = false;
			}

			// Dibujamos las ROIs y la plaza
			if (norm1 > umbralPresencia)
				rectangle(frame_vigilancia, ROIs1[i], rojo);
			else
				rectangle(frame_vigilancia, ROIs1[i], verde);
			// Dependiendo del tamaño del aparcamiento puede ser que no haya ROI2
			if (ROIs2[i].area() > 0)
			{
				if (norm2 > umbralPresencia)
					rectangle(frame_vigilancia, ROIs2[i], rojo);
				else
					rectangle(frame_vigilancia, ROIs2[i], verde);
			}

			if (plazaOcupada == true)
				rectangle(frame_vigilancia, plaza[i], rojo);
			else
				rectangle(frame_vigilancia, plaza[i], verde);
		}

		// Mostramos la imagen del aparcamiento captada por la cámara
		imshow("Vigilancia", frame_vigilancia);
		//imshow("motion", movimiento);

		// Actualizamos el frame cada 1 segundo. Salimos del programa pulsando la tecla esc
		esc = waitKey(30);
		if (esc == 27)
			break;
	}
	
	destroyAllWindows();
	cout << "\nHa seleccionado salir del programa." << endl;
	cout << "Pulse una tecla para continuar." << endl;
	getchar();
	return 0;
}

//////	FUNCIONES GLOBALES	//////

void onMouse(int event, int x, int y, int, void*)
{
	// Vigilando cuando pulsas el ratón (inicio del rectángulo) y cuando lo sueltas (final del mismo) podemos extraer el rectángulo
	// que contiene el objeto a rastrear
	switch (event)
	{
	// Evento pulsar boton del ratón
	case CV_EVENT_LBUTTONDOWN:
		pulsar = Point(x, y);
		break;
	case CV_EVENT_MOUSEMOVE:
		seleccion.x = MIN(pulsar.x, x);
		seleccion.y = MIN(pulsar.y, y);
		seleccion.width = std::abs(pulsar.x - x);
		seleccion.height = std::abs(pulsar.y - y);
		rectangle(frame, seleccion, Scalar(0, 255, 0));
		break;
	// Evento soltar botón del ratón
	case CV_EVENT_LBUTTONUP:
		soltar = Point(x, y);
		// Construimos la plaza de aparcamiento
		plaza[plazasSeleccionadas].x = MIN(pulsar.x, soltar.x);
		plaza[plazasSeleccionadas].y = MIN(pulsar.y, soltar.y);
		plaza[plazasSeleccionadas].width = std::abs(pulsar.x - soltar.x);
		plaza[plazasSeleccionadas].height = std::abs(pulsar.y - soltar.y);
		plazaSeleccionada = true;
		plazasSeleccionadas++;
		break;
	default:
		break;
	}
}