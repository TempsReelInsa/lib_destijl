/**
 * \file dcamera.c
 * \brief Source de la classe DCamera
 * \author P.-E. Hladik
 * \version 0.1
 * \date 28 novembre 2011
 *
 * Implémentation de la classe DCamera
 *
 */

#include "../headers/dcamera.h"
#include "../headers/dtools.h"

static void d_camera_init(DCamera*);

static void d_camera_init(DCamera *This) {
    This->print = d_camera_print;
    This->close = d_camera_close;
    This->get_frame = d_camera_get_frame;
    This->mCapture = NULL;
    This->mIndice = -1;
    This->open = d_camera_open;
}

DCamera* d_new_camera(void) {
    DCamera *This = malloc(sizeof (DCamera));
    if (This == NULL) {
        printf("\nAttempt to allocate DCamera failed.\n");
        exit(1);
    }
    if (!This) return NULL;
    d_camera_init(This);
    This->free = d_camera_free;
    return This;
}

void d_camera_print(DCamera *This) {
    printf("Camera{num:%i}\n", This->mIndice);
    // Remplissage des infos (indice de la caméra et résolution)
    /*oss << "Camera " << mIndice;
    oss << " (" << cvGetCaptureProperty(mCapture, CV_CAP_PROP_FRAME_WIDTH);
    oss << "x" << cvGetCaptureProperty(mCapture, CV_CAP_PROP_FRAME_HEIGHT) << ")";
     */
}

void d_camera_free(DCamera *This) {
    if (This->mCapture != NULL) cvReleaseCapture(&(This->mCapture));
    free(This);
}

int d_camera_open(DCamera *This) {
    return d_camera_open_number(This, This->mIndice);
}

int d_camera_open_number(DCamera *This, int number)
{
    // Création de la capture
    int ret = 0;
    This->mCapture = cvCreateCameraCapture(number);

    //TODO : Envoyer une erreur
    if (!This->mCapture) {
        ret = -1;
    } else {
        This->mIndice = number;
    }

    return ret;
}

void d_camera_close(DCamera *This) {
    if (This->mCapture != NULL) cvReleaseCapture(&(This->mCapture));
    This->mCapture = NULL;
    This->mIndice = -1;
}

int d_camera_get_frame(DCamera *This, DImage * img)
{
    int ret = 0;
    IplImage * frame;
    int i;

    for(i=0; i<5; i++)
    {
        cvGrabFrame(This->mCapture);
    }

    frame = cvRetrieveFrame(This->mCapture, 0);
    
    if (frame == NULL)
    {
        ret = 1;
    } else {
        d_image_set_ipl(img, frame);
    }

    return ret;
}

