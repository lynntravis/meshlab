/****************************************************************************
 * MeshLab                                                           o o     *
 * A versatile mm->cm processing toolbox                             o     o   *
 *                                                                _   O  _   *
 * Copyright(C) 2005                                                \/)\/    *
 * Visual Computing Lab                                            /\/|      *
 * ISTI - Italian National Research Council                           |      *
 *                                                                    \      *
 * All rights reserved.                                                      *
 *                                                                           *
 * This program is free software; you can redistribute it and/or modify      *   
 * it under the terms of the GNU General Public License as published by      *
 * the Free Software Foundation; either version 2 of the License, or         *
 * (at your option) any later version.                                       *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License (http://www.gnu.org/licenses/gpl.txt)          *
 * for more details.                                                         *
 *                                                                           *
 ****************************************************************************/
/****************************************************************************
  History
$Log: meshedit.cpp,v $
****************************************************************************/
#include <QtGui>

#include <math.h>
#include <stdlib.h>
#include <meshlab/glarea.h>
#include <meshlab/mainwindow.h>
#include "edit_ocme.h"

#include "ui_ocme.h"
#include <wrap/gl/picking.h>
#include<vcg/complex/trimesh/append.h>

using namespace std;
using namespace vcg;


#define _RELEASED_

OcmeEditPlugin::OcmeEditPlugin() {
		showTouched = false;
	qFont.setFamily("Helvetica");
	qFont.setPixelSize(12);    
	mm = NULL;
	ocme_bbox.SetNull();
	initialized = false;
	ocme_loaded = false;
	isDragging = false;
	isToSelect = false;
	Impostor::Gridsize()= 8; // *** TO DO: include in the database
}

const QString OcmeEditPlugin::Info()
{
	return tr("handle OCME.");
}
 
void OcmeEditPlugin::mousePressEvent(QMouseEvent *e, MeshModel &, GLArea * ) {
	pickx = e->pos().x();
	picky = e->pos().y();
	pick = true;
	start=e->pos();
	cur=start;
};
void OcmeEditPlugin::mouseMoveEvent(QMouseEvent *e, MeshModel &, GLArea * a) {
 isDragging = true;
 prev=cur;
 cur=e->pos();
 a->update();
};



void OcmeEditPlugin::mouseReleaseEvent(QMouseEvent * , MeshModel &   , GLArea *  )
{
		isDragging = false;
		isToSelect = true;

}

void DrawCellSel ( CellKey & ck, int mode = 0 );
void DrawCell ( CellKey & ck ){	DrawCellSel ( ck, 0 );}
vcg::Box3f GetViewVolumeBBox()
{
	vcg::Matrix44f mm,mp;
	glGetFloatv ( GL_PROJECTION_MATRIX,&mp[0][0] );
	glGetFloatv ( GL_MODELVIEW_MATRIX,&mm[0][0] );

	vcg::Transpose ( mp );
	vcg::Transpose ( mm );
	vcg::Invert ( mp );
	vcg::Invert ( mm );

	vcg::Box3f ubox;
	ubox.min = vcg::Point3f ( -1,-1,-1 );
	ubox.max = vcg::Point3f ( 1, 1, 1 );
	vcg::Box3f res;
	res.Add ( mm*mp,ubox );
	return res;

}

void OcmeEditPlugin::DrawCellsToEdit( ){

		// render all the cells only on the stencil buffer
		glColorMask(false,false,false,false);
		for(unsigned int i  = 0; i < cells_to_edit.size(); ++i)
			DrawCellSel(cells_to_edit[i]->key,1);
		glColorMask(true,true,true,true);

		glDepthFunc(GL_EQUAL);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glDisable(GL_LIGHTING);
		glColor4b(0,0,0,127);
		for(unsigned int i  = 0; i < cells_to_edit.size(); ++i)
			DrawCellSel(cells_to_edit[i]->key,1);

		glEnable(GL_LIGHTING);
		glDisable(GL_BLEND);
		glDepthFunc(GL_LESS);

}

void OcmeEditPlugin::Decorate(MeshModel &, GLArea * gla)
{
	rendering.lock();

	vcg::Color4b c;
	int lev;
	float stepf=1.f;
	vcg::Box3f ubox;
	ubox.min=vcg::Point3f ( -0.5,-0.5,-0.5 );
	ubox.max=-ubox.min;

	// rendering ocme (to be replaced with multires rendering )
	if ( !ocme_loaded )
	{
		glPushAttrib(GL_ALL_ATTRIB_BITS);
		glEnable(GL_COLOR_MATERIAL);
		glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);

		vcg::Box3f crn = GetViewVolumeBBox();
		lev = std::log ( crn.Diag() /10.f ) /std::log ( 2.0 );
		lev = std::max ( std::min ( 15,lev ),-15 );
		stepf = ( lev>0 ) ? ( 1<<lev ) :  1.f/ ( float ) ( 1<< ( -lev ) ) ;

		for ( int y = 0; y < 3; ++y ) crn.min[y] = floor ( crn.min[y]/stepf ) *stepf;
		for ( int y = 0; y < 3; ++y ) crn.max[y] = floor ( crn.max[y]/stepf ) *stepf;

		c = c.Scatter ( 32,lev+16 );
		glColor ( c );
		for ( float x = crn.min[0]-stepf; x < crn.max[0]+stepf; x+=stepf )
			for ( float y = crn.min[1]-stepf; y < crn.max[1]+stepf; y+=stepf )
				for ( float z = crn.min[2]-stepf; z < crn.max[2]+stepf; z+=stepf )
				{
					glPushMatrix();
					glTranslatef ( x+stepf*0.5,y+stepf*0.5,z+stepf*0.5 );
					glScalef ( stepf*0.125,stepf*0.125,stepf*0.125 );
					vcg::glBoxFlat ( ubox );
					glPopMatrix();
				}
		glPopAttrib();
	}else{

		ocme->Render();

		if(showTouched){
				for(unsigned int i  = 0; i < ocme->touched_cells.size(); ++i){
		 DrawCellSel( ocme->touched_cells[i],1);
		 ocme->GetCell(ocme->touched_cells[i],false)->impostor->Render(false);
	}
		}

		if(isToSelect){
				isToSelect = false;
				float xsel_min = (start.x()/(float)gla->width()-0.5f) *2.f;
				float xsel_max = (cur.x()/(float)gla->width()-0.5f) *2.f;
				float ysel_min = -(start.y()/(float)gla->height()-0.5f) *2.f;
				float ysel_max = -(cur.y()/(float)gla->height()-0.5f) *2.f;

				if(xsel_min>xsel_max) std::swap(xsel_min,xsel_max);
				if(ysel_min>ysel_max) std::swap(ysel_min,ysel_max);

				ocme->sel_corners[0] = vcg::Point4f(xsel_min,ysel_min,-1,1.0);
				ocme->sel_corners[1] = vcg::Point4f(xsel_max,ysel_min,-1,1.0);
				ocme->sel_corners[2] = vcg::Point4f(xsel_min,ysel_max,-1,1.0);
				ocme->sel_corners[3] = vcg::Point4f(xsel_max,ysel_max,-1,1.0);

				ocme->sel_corners[4] = vcg::Point4f(xsel_min,ysel_min, 1,1.0);
				ocme->sel_corners[5] = vcg::Point4f(xsel_max,ysel_min, 1,1.0);
				ocme->sel_corners[6] = vcg::Point4f(xsel_min,ysel_max, 1,1.0);
				ocme->sel_corners[7] = vcg::Point4f(xsel_max,ysel_max, 1,1.0);

				ocme->DeSelect(selected_cells);
				selected_cells.clear();
				ocme->Select(selected_cells);
				cells_to_edit = selected_cells;
				::RemoveDuplicates(cells_to_edit);
				updateButtonsState();
		}

	}

	// if mouse has been pressed do the picking
	if ( pick )
	{
		pick = false;
		std::vector<CellKey*> results;
		if ( vcg::Pick ( pickx, gla->height()-picky, all_keys,results, DrawCell /*DrawImpostor*/) )
		{
			CellKey cellkey  = *results[0];
			Cell *c = ocme->GetCell(cellkey,false);
			cells_to_edit.push_back(c);
		}
	}

	DrawCellsToEdit();
//	// draw the cells selected as sphere
//	for(unsigned int i  = 0; i < cells_to_edit.size(); ++i)
//		DrawCellSel(cells_to_edit[i]->key,1);

	if(isDragging)
			DrawXORRect(gla);


	rendering.unlock();

#ifndef _RELEASED_
	if(ocme_loaded){
			glPushAttrib ( GL_ALL_ATTRIB_BITS );
			glMatrixMode(GL_PROJECTION); glPushMatrix();glLoadIdentity();
			glMatrixMode(GL_MODELVIEW); glPushMatrix();glLoadIdentity();
			glDisable ( GL_DEPTH_TEST );
			glColor3f ( 1,1,1 );
			QString msg ( "#poly " );
			msg.append ( QString().setNum (  n_poly ) );
			gla->renderText ( 20, 15,  msg );

			msg= QString( "#cells " )+ QString().setNum(ocme->cells.size());
			gla->renderText ( 20, 30,  msg );

			msg= QString( "#roots " )+ QString().setNum(ocme->octree_roots.size());
			gla->renderText ( 20, 45,  msg );

			glMatrixMode(GL_PROJECTION); glPopMatrix();
			glMatrixMode(GL_MODELVIEW); glPopMatrix();
			glPopAttrib();
	}
#endif
}

void OcmeEditPlugin::drawFace(CMeshO::FacePointer , MeshModel &, GLArea * )
{
}

void OcmeEditPlugin::updateButtonsState(){

		odw->closeOcmPushButton->setEnabled(ocme_loaded);
		odw->addPushButton->setEnabled(ocme_loaded);
		odw->editPushButton->setEnabled(!this->cells_to_edit.empty());

		odw->loadOcmPushButton->setEnabled(!ocme_loaded);
		odw->createOcmPushButton->setEnabled(!ocme_loaded);
		//odw->markEditablePushButton->setEnabled(!mm->cm.face.empty());
}

bool OcmeEditPlugin::StartEdit(MeshModel &/*m*/, GLArea *_gla )
{
	STAT::Begin(N_STAT);
	/* patch to comply to current Mesdlab architecture*/
		if(this->initialized) { ocme_panel->show();return true;}

	gla = _gla;
	odw = new Ui::OcmeDockWidget ();
	ocme_panel  = new QDockWidget(gla);
	odw->setupUi(ocme_panel);

//	OcmeGlobals::FillNAFB<CMeshO>();

	ocme_panel->show();
	ocme_loaded = false;
	gla->setCursor(QCursor(QPixmap(":/images/cur_ocme.png"),1,1));
	QObject::connect(odw->loadOcmPushButton,SIGNAL(clicked()),this,SLOT(loadOcm()));
	QObject::connect(odw->createOcmPushButton,SIGNAL(clicked()),this,SLOT(createOcm()));
	QObject::connect(odw->closeOcmPushButton,SIGNAL(clicked()),this,SLOT(closeOcm()));
	QObject::connect(odw->editPushButton,SIGNAL(clicked()),this,SLOT(edit()));
	QObject::connect(odw->markEditablePushButton ,SIGNAL(clicked() ),this,SLOT( markEditable() ));
	QObject::connect(odw->dropSelectionPushButton,SIGNAL(clicked()),this,SLOT(drop()));
	QObject::connect(odw->commitPushButton,SIGNAL(clicked()),this,SLOT(commit()));
	QObject::connect(odw->addPushButton,SIGNAL(clicked()),this,SLOT(add()));
	QObject::connect(odw->fillAttrMeshPushButton ,SIGNAL(clicked() ),this,SLOT( fillMeshAttribute()));
	QObject::connect(odw->fillAttrOcmPushButton ,SIGNAL(clicked() ),this,SLOT( fillOcmAttribute()));
	QObject::connect(odw->tri2ocmPushButton ,SIGNAL(clicked() ),this,SLOT( tri2ocmAttribute()));
	QObject::connect(odw->ocm2triPushButton ,SIGNAL(clicked() ),this,SLOT( ocm2triAttribute()));
	QObject::connect(odw->refreshImpostorsPushButton ,SIGNAL(clicked() ),this,SLOT( refreshImpostors()));
	QObject::connect(odw->toggleExtrPushButton ,SIGNAL(clicked() ),this,SLOT( toggleExtraction() ));
	QObject::connect(odw->toggleShowTouchedPushButton ,SIGNAL(clicked() ),this,SLOT( toggleShowTouched() ));
	QObject::connect(odw->addFromDiskPushButton ,SIGNAL(clicked() ),this,SLOT( addFromDisk() ));


	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), gla, SLOT(updateGL()));
	timer->start(300);

	// store current trackball
	curr_track.track.sca = gla->trackball.track.sca;
	curr_track.track.tra = gla->trackball.track.tra;

	if(!ocme_bbox.IsNull()) setTrackBall();

	initialized = true;

#ifdef _RELEASED_
	odw->commitPushButton->setEnabled(false);
	odw->addFromDiskPushButton->setEnabled(false);
	odw->refreshImpostorsPushButton->hide();
	odw->toggleExtrPushButton->hide();
	odw->toggleShowTouchedPushButton->hide();

	odw->fillAttrMeshPushButton->setEnabled(false);
	odw->fillAttrOcmPushButton->setEnabled(false);
	odw->ocm2triPushButton->setEnabled(false);
	odw->tri2ocmPushButton->setEnabled(false);

#endif

	updateButtonsState();

	return true;
}

void OcmeEditPlugin::EndEdit(MeshModel &/*m*/, GLArea *gla )
{
	ocme_panel->hide();
}

void OcmeEditPlugin::setTrackBall(){
 gla->trackball.Reset();
 float newScale= 3.0f/ocme_bbox.Diag();
 gla->trackball.track.sca = newScale;
 gla->trackball.track.tra =  -ocme_bbox.Center();

}
void OcmeEditPlugin::	refreshImpostors(){
		ocme->BuildImpostorsHierarchy();
		 n_poly  =0;
}
void OcmeEditPlugin::toggleExtraction(){
ocme->renderParams.visitOn = !ocme->renderParams.visitOn;
}
void OcmeEditPlugin::toggleShowTouched(){
showTouched = !showTouched;
}
void OcmeEditPlugin::UpdateBoundingBox(){
all_keys.clear();
ocme_bbox.SetNull();
OCME::CellsIterator ci;

// phase 1. recompute the bounding box
for ( ci = ocme->cells.begin(); ci != ocme->cells.end(); ++ci )
	if ( !  ( *ci ).second->IsEmpty() )
	{
		ocme_bbox.Add ( ( *ci ).first.GP3f() );
		all_keys.push_back ( ( *ci ).first );
	}
}

void OcmeEditPlugin::fillMeshAttribute(){

	{
	std::vector<std::string> name;
	CMeshO::VertexType::Name(name);
	for(std::vector<std::string>::iterator i = name.begin(); i != name.end(); ++i)
		odw->meshAttrListWidget->addItem((std::string("vertex::").append(*i)).c_str());
	}

	{
	std::vector<std::string> name;
	CMeshO::FaceType::Name(name);
	for(std::vector<std::string>::iterator i = name.begin(); i != name.end(); ++i)
		odw->meshAttrListWidget->addItem((std::string("face::").append(*i)).c_str());
	}

}
void OcmeEditPlugin::fillOcmAttribute(){
	AttributeMapper am;
	ocme->GetCellsAttributes(cells_to_edit,am);

	for(unsigned int i  = 0; i < am.vert_attrs.size();++i)
		odw->ocmeAttrListWidget->addItem( QString("vertex::").append(QString(am.vert_attrs[i].c_str())));
	for(unsigned int i  = 0; i < am.face_attrs.size();++i)
		odw->ocmeAttrListWidget->addItem( QString("face::").append(QString(am.face_attrs[i].c_str())));
}
void OcmeEditPlugin::tri2ocmAttribute(){
	odw->ocmeAttrListWidget->addItem(odw->meshAttrListWidget->currentItem()->text());
}

void OcmeEditPlugin::ocm2triAttribute(){
	odw->meshAttrListWidget->addItem(odw->ocmeAttrListWidget->currentItem()->text());
}




void OcmeEditPlugin::loadOcm(){
	ocm_name = QFileDialog::getOpenFileName((QWidget*)0,
					   tr("Open Ocm"), QDir::currentPath(),
#ifndef NO_BERKELEY
					   tr("Ocm file (*.ocm )"));
#else
							tr("Ocm file (*.socm )"));
#endif
	if(!ocm_name.isEmpty()){
		ocme  = new OCME();
		ocme->params.side_factor = 50; // READ IT FROM THE FILEEEEEEEEE
		ocme->InitRender();
		ocme->renderParams.memory_limit_in_core = 20;
		ocme->Open ( ocm_name.toAscii() );

#ifdef _RELEASED_
		refreshImpostors();
#endif

		UpdateBoundingBox();
		setTrackBall();
		mm = new MeshModel("Ocm patch");
		gla->meshDoc->addNewMesh("Ocm patch",mm);
		ocme_loaded = true;
		updateButtonsState();

	}
}


void OcmeEditPlugin::addFromDisk(){
		QStringList list = QFileDialog::getOpenFileNames((QWidget*)0,tr("Open ply/aln"),QDir::currentPath(),
																	tr("Mesh file (*.ply *.aln)"));
			for ( QStringList::Iterator it = list.begin();
			it != list.end(); ++it){
			}
			updateButtonsState();

}

void OcmeEditPlugin::resetPlugin(){
this->ocm_name = QString();
//OcmeGlobals::NAFB().clear();
delete this->ocme;
gla->meshDoc->delMesh(this->mm);
this->mm = 0;
this->all_keys.clear();
this->cells_to_edit.clear();
updateButtonsState();

}

void OcmeEditPlugin::closeOcm(){
	rendering.lock();
	ocme->renderCache.Finish();
	ocme->RemoveEmptyCells();
	ocme->Close(true);
	ocme_loaded = false;
	rendering.unlock();
	resetPlugin();
	updateButtonsState();

}

void OcmeEditPlugin::createOcm(){
	ocm_name = QFileDialog::getSaveFileName((QWidget*)0,
					   tr("Open Ocm"), QDir::currentPath(),
#ifndef NO_BERKELEY
						 tr("Ocm file (*.ocm )"));
#else
							tr("Ocm file (*.socm )"));
#endif
	if(!ocm_name.isEmpty()){
		ocme  = new OCME();
		ocme->Create(ocm_name.toAscii());
		ocme->InitRender();

		mm = new MeshModel("Ocm patch");
		gla->meshDoc->addNewMesh("Ocm patch",mm);

		/* paramters to be exposed somehow later  on */
		ocme->params.side_factor = 20;
		ocme->oce.cache_policy->memory_limit  = /*cache_memory_limit*/ 100 * (1<<20);
		ocme_bbox.SetNull();
		/* */
		ocme_loaded = true;

		updateButtonsState();

	}
}
void OcmeEditPlugin::drop(){
		ocme->renderCache.controller.pause();
		mm->cm.Clear();
		cells_to_edit.clear();
		ocme->DropEdited();
		ocme->renderCache.controller.resume();
}

void OcmeEditPlugin::markEditable(){
		CMeshO::  PerVertexAttributeHandle<unsigned char>  lockedV =
				vcg::tri::Allocator<CMeshO>::  GetPerVertexAttribute<unsigned char> (mm->cm,"ocme_locked");

		CMeshO::  PerFaceAttributeHandle<unsigned char>  lockedF =
				vcg::tri::Allocator<CMeshO>::  GetPerFaceAttribute<unsigned char> (mm->cm,"ocme_locked");

		for(CMeshO::VertexIterator vi = mm->cm.vert.begin(); vi != mm->cm.vert.end(); ++vi )
				if(!(*vi).IsD())
						if(!lockedV[*vi]) (*vi).SetS();

		for(CMeshO::FaceIterator fi = mm->cm.face.begin(); fi != mm->cm.face.end(); ++fi )
				if(!(*fi).IsD()){
						bool ed = !lockedF[*fi];
						for(unsigned int i = 0; i < (*fi).VN() ; ++i)
								 ed = ed &&  !lockedV[(*fi).V(i)];
						if(ed) (*fi).SetS();
						}
}

void OcmeEditPlugin::edit(){

	ocme->renderCache.controller.pause();

	mm->cm.Clear();

	AttributeMapper attrMapper;
	for(int i = 0;  i < odw->meshAttrListWidget->count();++i ) {
		QString nameAttr = odw->meshAttrListWidget->item(i)->text();

		if(nameAttr.contains(QString("vertex::")) && (nameAttr!=QString("vertex::Coord3f")) && (nameAttr!=QString("vertex::Coord3d")))
			attrMapper.vert_attrs.push_back(nameAttr.remove(QString("vertex::")).toStdString());
		else
		if(nameAttr.contains(QString("face::")) && (nameAttr!=QString("face::VertexRef")))
			attrMapper.face_attrs.push_back(nameAttr.remove(QString("face::")).toStdString());
		else
			assert(0);
	}

/* 	mm->cm.vert.EnableColor(); */
	for(unsigned int i = 0; i < attrMapper.vert_attrs.size();++i){
		if(attrMapper.vert_attrs[i]==std::string("Normal3f") ) mm->cm.vert.EnableNormal();else
		if(attrMapper.vert_attrs[i]==std::string("Color4b") ) mm->cm.vert.EnableColor();else
		if(attrMapper.vert_attrs[i]==std::string("Qualityf") ) mm->cm.vert.EnableQuality(); else
		if(attrMapper.vert_attrs[i]==std::string("TexCoord") ) mm->cm.vert.EnableTexCoord();
	}

	for(unsigned int i = 0; i < attrMapper.face_attrs.size();++i){
		if(attrMapper.face_attrs[i]==std::string("Normal3f") ) mm->cm.face.EnableNormal();else
		if(attrMapper.face_attrs[i]==std::string("Color4b") ) mm->cm.face.EnableColor();else
		if(attrMapper.face_attrs[i]==std::string("Qualityf") ) mm->cm.face.EnableQuality();
	}



	ocme->Edit(cells_to_edit,mm->cm,attrMapper);
	ocme->DeSelect(this->cells_to_edit);
	cells_to_edit.clear();

	vcg::tri::UpdateNormals<CMeshO>::PerVertexPerFace ( mm->cm );
//
//	CMeshO::  PerFaceAttributeHandle<GIndex>  gposf =
//			vcg::tri::Allocator<CMeshO>::  GetPerFaceAttribute<GIndex> (mm->cm,"ocme_gindex");
//
//	CMeshO::  PerVertexAttributeHandle<unsigned char>  lockedV =
//			vcg::tri::Allocator<CMeshO>::  GetPerVertexAttribute<unsigned char> (mm->cm,"ocme_locked");
//
//	CMeshO::  PerFaceAttributeHandle<unsigned char>  lockedF =
//			vcg::tri::Allocator<CMeshO>::  GetPerFaceAttribute<unsigned char> (mm->cm,"ocme_locked");
//
//
//	for(unsigned int i = 0; i < mm->cm.face.size();++i)
//			if(!mm->cm.face[i].IsD())
//					if( lockedF[i] ) {
//			for(unsigned int fi = 0; fi < 3; ++ fi ){
//								lockedV[mm->cm.face[i].V(fi)] = 1;
//								mm->cm.face[i].V(fi)->SetS();
//		}
//			mm->cm.face[i].SetS();
//	}
//
//	for(unsigned int i = 0; i < mm->cm.vert.size();++i)
//			if( lockedV[i] ) mm->cm.vert[i].SetS();



	vcg::tri::UpdateBounding<CMeshO>::Box(mm->cm);

	ocme->renderCache.controller.resume();

}

void OcmeEditPlugin::commit(){
	rendering.lock();
	ocme->renderCache.Finish();
	ocme->Commit ( mm->cm );
	UpdateBoundingBox();
	mm->cm.Clear();
	ocme->renderCache.Start();
	rendering.unlock();
}

void OcmeEditPlugin::add(){
	if(gla->meshDoc->mm()  == mm)
			return;

	// fetch the mesh components selected to be inserted in the ocm database
	AttributeMapper attrMapper;
	for(unsigned int i = 0;  i < odw->ocmeAttrListWidget->count();++i ) {
		QString nameAttr = odw->ocmeAttrListWidget->item(i)->text();

		if(nameAttr.contains(QString("vertex::")) && (nameAttr!=QString("vertex::Coord3f")) && (nameAttr!=QString("vertex::Coord3d")))
			attrMapper.vert_attrs.push_back(nameAttr.remove(QString("vertex::")).toStdString());
		else
		if(nameAttr.contains(QString("face::")) && (nameAttr!=QString("face::VertexRef")))
			attrMapper.face_attrs.push_back(nameAttr.remove(QString("face::")).toStdString());
		else
			assert(0);
	}


	ocme->AddMesh( gla->meshDoc->mm()->cm,attrMapper );

	ocme->BuildImpostorsHierarchy(ocme->added_cells);

	gla->meshDoc->delMesh( gla->meshDoc->mm());
	UpdateBoundingBox();
	setTrackBall();
	Log("adding selected layer");
}

/* selection */
void OcmeEditPlugin::DrawXORRect(GLArea * gla)
{
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0,gla->curSiz.width(),gla->curSiz.height(),0,-1,1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_COLOR_LOGIC_OP);
	glLogicOp(GL_XOR);
	glColor3f(1,1,1);

	glBegin(GL_LINE_LOOP);
		glVertex2f(start.x(),start.y());
		glVertex2f(cur.x(),start.y());
		glVertex2f(cur.x(),cur.y());
		glVertex2f(start.x(),cur.y());
	glEnd();
	glDisable(GL_LOGIC_OP);

	glColor3f(1.f,0.0,0.0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);
	glVertex2f(start.x(),start.y());
	glVertex2f(cur.x(),start.y());
	glVertex2f(cur.x(),cur.y());
	glVertex2f(start.x(),cur.y());
	glEnd();

	// Closing 2D
	glPopAttrib();
	glPopMatrix(); // restore modelview
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

}