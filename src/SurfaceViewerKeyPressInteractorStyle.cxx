/*
**    CPE Lyon
**    2018 Camille FARINEAU / Nicolas Ranc
**    Projet Majeur - Virtual Endoscopy
**
**    SurfaceViewerKeyPressInteractorStyle.cxx
**    Interactor Style for the 3D Viewer (or SurfaceViewer) : handle key event (to move the camera in the scene)
*/

#include "SurfaceViewerKeyPressInteractorStyle.h"

// Template for image value reading
template<typename T>
void vtkValueMessageTemplate(vtkImageData* image, int* position,
                             std::string& message)
{
  T* tuple = ((T*)image->GetScalarPointer(position));
  int components = image->GetNumberOfScalarComponents();
  for (int c = 0; c < components; ++c)
    {
    message += vtkVariant(tuple[c]).ToString();
    if (c != (components - 1))
      {
      message += ", ";
      }
    }
  message += " )";
}


/*------------------------------------------------------------------------*\
 * SurfaceViewerKeyPressInteractorStyle::SurfaceViewerKeyPressInteractorStyle()
 * Constructor
\*------------------------------------------------------------------------*/
SurfaceViewerKeyPressInteractorStyle::SurfaceViewerKeyPressInteractorStyle()
{
    this->Camera     = NULL;
    this->intersectionPolyDataFilter = vtkSmartPointer<vtkIntersectionPolyDataFilter>::New();
    this->cellIdArray = vtkSmartPointer<vtkIdTypeArray>::New();
    this->node = vtkSmartPointer<vtkSelectionNode>::New();
    this->selection = vtkSmartPointer<vtkSelection>::New();
    this->extractSelection = vtkSmartPointer<vtkExtractSelection>::New();
    this->selectedCells = vtkSmartPointer<vtkUnstructuredGrid>::New();
    this->geometryFilter = vtkSmartPointer<vtkGeometryFilter>::New();
    this->nb_inter = 0;
    this->collision = true;
}

/*------------------------------------------------------------------------*\
 * SurfaceViewerKeyPressInteractorStyle::SurfaceViewerKeyPressInteractorStyle()
 * Destructor
\*------------------------------------------------------------------------*/
SurfaceViewerKeyPressInteractorStyle::~SurfaceViewerKeyPressInteractorStyle()
{
    this->Camera     = NULL;
}

/*------------------------------------------------------------------------*\
 * SurfaceViewerKeyPressInteractorStyle::SetCamera
 * Set the camera use in the scene
 * Param: camera_: a vtkCamera
\*------------------------------------------------------------------------*/
void SurfaceViewerKeyPressInteractorStyle::SetCamera(const vtkSmartPointer<vtkCamera>& camera_){
    Camera=camera_;
}

/*------------------------------------------------------------------------*\
 * SurfaceViewerKeyPressInteractorStyle::SetInteractor
 * Set the interactor of the RenderWindow. Useful to interact with the scene
 * Param: an interactor
\*------------------------------------------------------------------------*/
void SurfaceViewerKeyPressInteractorStyle::SetInteractor(const vtkSmartPointer<vtkRenderWindowInteractor>& interactor){
    this->Interactor=interactor;
}

/*------------------------------------------------------------------------*\
 * SurfaceViewerKeyPressInteractorStyle::SetSurface()
 * Set the iso-surface created with Marching cubes (useful for collision between the camera bbox and the surface)
 * Param: surface: a vtkPolyData
\*------------------------------------------------------------------------*/
void SurfaceViewerKeyPressInteractorStyle::SetSurface(const vtkSmartPointer<vtkPolyData> &surface){
    this->Surface=surface;
}

/*------------------------------------------------------------------------*\
 * SurfaceViewerKeyPressInteractorStyle::SetSurfaceCollision()
 * Set the iso-surface specific for collision
 * Param: surface: a vtkPolyData
\*------------------------------------------------------------------------*/
void SurfaceViewerKeyPressInteractorStyle::SetSurfaceCollision(const vtkSmartPointer<vtkPolyDataAlgorithm> &surface_col){
    this->Surface_col = surface_col;
}


/*------------------------------------------------------------------------*\
 * SurfaceViewerKeyPressInteractorStyle::SetSphere()
 * Set the sphere that is used as a bbox for the camera (for the collision)
 * Param: sphere: a vtkSphereSource
\*------------------------------------------------------------------------*/
void SurfaceViewerKeyPressInteractorStyle::SetSphere(const vtkSmartPointer<vtkSphereSource>& sphere){
    this->Sphere=sphere;
}


/*------------------------------------------------------------------------*\
 * SurfaceViewerKeyPressInteractorStyle::SetInteractionPolyDataFilter()
 * Set the 2 PolyData that will be used by the InteractionFilter (the bbox of the camera and the surface of the object of interest)
\*------------------------------------------------------------------------*/
void SurfaceViewerKeyPressInteractorStyle::SetIntersectionPolyDataFilter()
{
    // Set the first PolyData as the bbox of the camera (PolyData of the sphere)
    intersectionPolyDataFilter->SetInputData(0, Sphere->GetOutput());
    // Set the second PolyData as the surface of interest
    intersectionPolyDataFilter->SetInputData(1, Surface);
    // Update the filter
    intersectionPolyDataFilter->Update();
    //std::cout<<"Update surface for intersection"<<std::endl;
}


/*------------------------------------------------------------------------*\
 * SurfaceViewerKeyPressInteractorStyle::OnKeyPress()
 * Event on key press
\*------------------------------------------------------------------------*/
void SurfaceViewerKeyPressInteractorStyle::OnKeyPress()
{
    // Get the keypress
    vtkRenderWindowInteractor *rwi = this->Interactor;
    std::string key = rwi->GetKeySym();

    // Set the focal point
    Camera->SetDistance(1);

    // Handle UP arrow key
    if(key == "Up")
    {
        // Camera will look upward
        Camera->Pitch(1);
    }

    // Handle Down arrow key
    if(key == "Down")
    {
        // Camera will look backward
        Camera->Pitch(-1);
    }

    // Handle Left arrow key
    if(key == "Left")
    {
    Camera->Azimuth(1);
    }

    // Handle Right arrow key
    if(key == "Right")
    {
    Camera->Azimuth(-1);
    }

    // For each movement backward or upward
    if((key == "z" || key=="Z" || key == "s" || key=="S") && collision)
    {
        /******************************************************/
        // This sample of code is in charge of create a surface from the cells (triangles) near the camera
        // This way it will look for collision between the bbox of the camera and the surface only with these triangles that are near the camera
        // and not all the triangles of the surface
        // It is freely inspirated from this code : https://www.vtk.org/Wiki/VTK/Examples/Cxx/PolyData/ExtractSelectionCells

        // Create a cell locator
        vtkSmartPointer<vtkCellLocator> cellLocator = vtkSmartPointer<vtkCellLocator>::New();

        // Set the data as the surface
        cellLocator->SetDataSet(Surface_col->GetOutput());
        // Create the cell locator
        cellLocator->BuildLocator();

        // Bounding box of the camera: cube where the camera is at the center
        // This code will look for all cells inside this cube
        double* bounds = new double[6];
        bounds[0] = Camera->GetPosition()[0]-1.0;
        bounds[1] = Camera->GetPosition()[0]+1.0;
        bounds[2] = Camera->GetPosition()[1]-1.0;
        bounds[3] = Camera->GetPosition()[1]+1.0;
        bounds[4] = Camera->GetPosition()[2]-1.0;
        bounds[5] = Camera->GetPosition()[2]+1.0;

        // Create a list of Ids
        vtkIdList* cellIdList = vtkIdList::New();

        // Find the cell of the surface inside the bbox of the camera
        // This function will populate the cellIdList
        cellLocator->FindCellsWithinBounds(bounds,cellIdList);

        //std::cout<<"nb cell: "<<cellIdList->GetNumberOfIds()<<std::endl;
        cellIdArray->SetNumberOfComponents(1);

        // Put all Ids we found in the list inside this array
        for(unsigned int i = 0; i < cellIdList->GetNumberOfIds(); i++)
        {
            cellIdArray->InsertNextValue(cellIdList->GetId(i));
        }

        node->SetFieldType(vtkSelectionNode::CELL);
        node->SetContentType(vtkSelectionNode::INDICES);
        // Set the list of Ids to the node
        node->SetSelectionList(cellIdArray);

        selection->AddNode(node);

        // Set the input connection to the surface in question
        extractSelection->SetInputConnection(0, Surface_col->GetOutputPort());
        // The data are the selection of cells inside this surface
        extractSelection->SetInputData(1, selection);
        extractSelection->Update();
        // Get all the cells that have been selected
        selectedCells->ShallowCopy(extractSelection->GetOutput());

//        std::cout << "There are " << selectedCells->GetNumberOfPoints()
//                  << " points in the selection." << std::endl;
//        std::cout << "There are " << selectedCells->GetNumberOfCells()
//                  << " cells in the selection." << std::endl;
        if(selectedCells->GetNumberOfCells() >= 1)
        {
            // Set the grid as data
            geometryFilter->SetInputData(selectedCells);
            geometryFilter->Update();
            // Get the PolyData created from the grid
            nearestSurface = geometryFilter->GetOutput();

            // Set the surface for the collision as this new limited surface containing the nearest cells from the camera
            this->SetSurface(nearestSurface);
            // Update the InteractionFilter
            this->SetIntersectionPolyDataFilter();
        }

        // Handle z key: move forward
        if(key == "z" || key=="Z")
        {
            // Move forward
            Camera->Dolly(5);
            // Set the focal point of the camera
            Camera->SetDistance(1);

            if(selectedCells->GetNumberOfCells() >= 1)
            {
                // Set the bbox of the camera at the correct location
                Sphere->SetCenter(Camera->GetPosition());

                // Render everything
                this->Interactor->GetRenderWindow()->Render();

                // Update the IntersectionFilter with the new PolyData
                intersectionPolyDataFilter->Update();

                // Get the number of intersections
                nb_inter = intersectionPolyDataFilter->GetNumberOfIntersectionPoints();
                //std::cout<<"Inters: "<<nb_inter<<std::endl;

                // If there are some intersections
                // Then move backward to get back to the old position
                // This method is a work-in-progress: it would be better to test the interaction at the new position with another camera object
                // and then move the real camera only if there are no intersection in the position
                if(nb_inter > 0)
                {
                    // Move backward
                    Camera->Dolly(0.3);
                    // Set the focal point of the camera
                    Camera->SetDistance(1);
                    // Render everything
                    this->Interactor->GetRenderWindow()->Render();
                }
            }
        }

        // Handle s key: move backward
        if(key == "s" || key == "S")
        {
            // Move backward
            Camera->Dolly(0.6);

            // Set the focal point of the camera
            Camera->SetDistance(1);

            if(selectedCells->GetNumberOfCells() >= 1)
            {
                // Set the bbox of the camera at the correct location
                Sphere->SetCenter(Camera->GetPosition());

                // Render everything
                this->Interactor->GetRenderWindow()->Render();

                // Update the IntersectionFilter with the new PolyData
                intersectionPolyDataFilter->Update();

                // Get the number of intersections
                nb_inter = intersectionPolyDataFilter->GetNumberOfIntersectionPoints();
                //std::cout<<"Inters: "<<nb_inter<<std::endl;

                // If there are some intersections
                // Then move forward to get back to the old position
                // This method is a work-in-progress: it would be better to test the interaction at the new position with another camera object
                // and then move the real camera only if there are no intersection in the position
                if(nb_inter > 0)
                {
                    // Move forward
                    Camera->Dolly(10);

                    // Set the focal point of the camera
                    Camera->SetDistance(1);

                    // Render everything
                    this->Interactor->GetRenderWindow()->Render();
                }
            }
        }
    }

    if(!collision){
        if(key == "z" || key=="Z"){

            // Move forward
            Camera->Dolly(5);
            // Set the focal point of the camera
            Camera->SetDistance(1);
        }
        if(key == "s" || key == "S"){

            // Move backward
            Camera->Dolly(0.6);

            // Set the focal point of the camera
            Camera->SetDistance(1);
        }
    }

    // Handle escape key
    if(key == "Escape")
    {
        // Quit app
        exit(0);
    }

    // Set Clipping Range
    double dis[2]={0.5,1000.0};
    Camera->SetClippingRange(dis);

    // Set the focal point of the camera
    Camera->SetDistance(1);

    // Move the bbox of the camera accordingly with the camera
    Sphere->SetCenter(Camera->GetPosition());

    // Render everything
    this->Interactor->GetRenderWindow()->Render();

 //Handle default VTK key press
 // vtkInteractorStyleTrackballCamera::OnKeyPress();
}
